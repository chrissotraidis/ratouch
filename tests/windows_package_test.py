"""Keep inherited Windows archives attributed and checked before upload."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest
import zipfile


ROOT = Path(__file__).resolve().parents[1]
AUDITOR = "f0efbb738cac934559e986143343dc8720a6fb1d"


class WindowsPackageTests(unittest.TestCase):
    def test_each_archive_includes_project_notices(self):
        for filename in ("windows.yml", "mingw.yml"):
            text = (ROOT / ".github/workflows" / filename).read_text()
            commands = re.findall(r"^\s+7z a artifact/[^\n]+", text, re.MULTILINE)
            self.assertTrue(commands, filename)
            for command in commands:
                with self.subTest(workflow=filename, command=command):
                    self.assertIn("./License.txt", command)
                    self.assertIn("./RIGHTS_AND_LICENSES.md", command)
                    self.assertIn("./licenses", command)

    def test_each_upload_follows_the_pinned_content_check(self):
        for filename in ("windows.yml", "mingw.yml"):
            text = (ROOT / ".github/workflows" / filename).read_text()
            jobs = re.split(r"(?m)^  [A-Za-z_][A-Za-z0-9_-]*:\n", text.split("\njobs:\n", 1)[1])[1:]
            self.assertTrue(jobs, filename)
            for job in jobs:
                with self.subTest(workflow=filename):
                    self.assertIn("repository: chrissotraidis/padmint", job)
                    self.assertIn("ref: " + AUDITOR, job)
                    self.assertIn("persist-credentials: false", job)
                    self.assertIn("python3 -B -m padmint audit", job)
                    self.assertIn('"$GITHUB_WORKSPACE"/artifact/*.zip', job)
                    gate = job.split("    - name: Audit staged Windows archives", 1)[1].split("    - name: Upload artifact", 1)[0]
                    upload = job.split("    - name: Upload artifact", 1)[1].split("    - name:", 1)[0]
                    self.assertEqual(re.findall(r"(?m)^      if: (.+)$", gate),
                                     re.findall(r"(?m)^      if: (.+)$", upload))
                    self.assertEqual(re.findall(r"(?m)^      run: (.+)$", gate),
                                     ['python3 -B -m padmint audit "$GITHUB_WORKSPACE"/artifact/*.zip'])
                    self.assertIn("working-directory: .ci-padmint", gate)
                    audit = job.index("python3 -B -m padmint audit")
                    for upload in re.finditer(r"uses: (?:actions/upload-artifact|softprops/action-gh-release)@", job):
                        self.assertLess(audit, upload.start())
                    self.assertNotRegex(job, r"continue-on-error:\s*true")

    def test_dependency_notices_are_staged_before_packaging(self):
        for filename in ("windows.yml", "mingw.yml"):
            text = (ROOT / ".github/workflows" / filename).read_text()
            jobs = re.split(r"(?m)^  [A-Za-z_][A-Za-z0-9_-]*:\n", text.split("\njobs:\n", 1)[1])[1:]
            for job in jobs:
                with self.subTest(workflow=filename):
                    self.assertIn("    - name: Stage dependency notices", job)
                    stage = job.split("    - name: Stage dependency notices", 1)[1].split("    - name:", 1)[0]
                    self.assertIn("cp -R tools/miniposix licenses/miniposix", stage)
                    self.assertLess(job.index("Stage dependency notices"), job.index("Create archives"))
                    for command in re.findall(r"^\s+7z a artifact/[^\n]+", job, re.MULTILINE):
                        self.assertIn("./licenses", command)
                    if "SDL2.dll" in job:
                        self.assertIn("SDL2-2.0.12/COPYING.txt", stage)
                        self.assertIn("SDL2-2.0.12/README-SDL.txt", stage)
                        self.assertIn("openal-soft-1.21.0-bin/COPYING", stage)
                        self.assertIn("licenses/SDL2", stage)
                        self.assertIn("licenses/OpenAL-Soft", stage)
                    self.assertNotIn("|| true", stage)

    @unittest.skipUnless(shutil.which("7z"), "7z is needed for the packaging replay")
    def test_real_archive_command_preserves_payload_and_notices(self):
        # The real remaster command with synthetic DLLs, not an engine build.
        text = (ROOT / ".github/workflows/windows.yml").read_text()
        command = re.search(r"(?m)^\s+7z a artifact/[^\n]+", text).group().strip()
        command = command.replace("${{ steps.gitinfo.outputs.sha_short }}", "fixture")
        with tempfile.TemporaryDirectory(prefix="ratouch-package-test-") as folder:
            path = Path(folder)
            (path / "artifact").mkdir()
            build = path / "build/remaster/RelWithDebInfo"
            build.mkdir(parents=True)
            for name in ("RedAlert.dll", "TiberianDawn.dll"):
                (build / name).write_bytes(b"synthetic packaging payload, not an app")
            for name in ("License.txt", "RIGHTS_AND_LICENSES.md"):
                (path / name).write_bytes((ROOT / name).read_bytes())
            (path / "licenses").mkdir()
            (path / "licenses/notice.txt").write_bytes(b"synthetic dependency notice")
            subprocess.run(command.split(), cwd=path, check=True, stdout=subprocess.DEVNULL)
            with zipfile.ZipFile(next((path / "artifact").glob("*.zip"))) as archive:
                self.assertIsNone(archive.testzip())
                self.assertEqual(archive.read("licenses/notice.txt"), b"synthetic dependency notice")
                for name in ("RedAlert.dll", "TiberianDawn.dll"):
                    self.assertEqual(archive.read(name), (build / name).read_bytes())
                for name in ("License.txt", "RIGHTS_AND_LICENSES.md"):
                    self.assertEqual(archive.read(name), (ROOT / name).read_bytes())


if __name__ == "__main__":
    unittest.main()
