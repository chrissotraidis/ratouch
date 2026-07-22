# Local asset provenance receipt

This document records the non-payload facts used to reproduce the first-playable validation. It does not grant redistribution rights and it does not place any commercial game data in Git.

## Source

- Product source: user-supplied Steam installation
- Steam app ID: `2229840`
- Local validation date: 2026-07-21
- Local payload location: `ref/Command & Conquer Red Alert/`
- Repository policy: `ref/.gitignore` ignores every payload file and permits only the folder README and ignore policy

## Deterministic Steam mapping

| Source file | Bytes | SHA-256 | Imported path |
| --- | ---: | --- | --- |
| `EXPAND.MIX` | 458242 | `e144753593161f867a26428f901a09de5cb8a71bab6e690dd9ca727e76d4f724` | `EXPAND.MIX` |
| `EXPAND2.MIX` | 469922 | `e379b23ce6c7af9d4f7469e10b788210124cb92e8b6e3978b9569802edfdfc9a` | `EXPAND2.MIX` |
| `HIRES1.MIX` | 90264 | `48c407f80f1fdbc86ac2689c00339927b2127a758871e998c9942b7a7d93e07d` | `HIRES1.MIX` |
| `LORES1.MIX` | 57076 | `5b83e8d731fc78041647f19adbb82f4e71cc09b837d2492b8f2a0520e11de641` | `LORES1.MIX` |
| `MAIN1.MIX` | 454605294 | `512beab10095f2422498f16ce468fca613bf6bec2a6257bbc18a1d01691d1482` | `allied/MAIN.MIX` |
| `MAIN2.MIX` | 500577414 | `cbcddf7fc75b2924728a2698307b97605557528b99e5f175d3d5de8f6c96de3c` | `soviet/MAIN.MIX` |
| `MAIN3.MIX` | 236034607 | `f747654b0ff09584086460d1b74ba400c2cc4dd9d467e27ca38c99443c45dae2` | `counterstrike/MAIN.MIX` |
| `MAIN4.MIX` | 270673111 | `1bc68a3509a762730a7bfedca132413a5d2ab3a883da8dbb4233b32366baa72f` | `aftermath/MAIN.MIX` |
| `REDALERT.MIX` | 25046328 | `ad5ad68a08d1d6bb073324e91beb02543deeb9e1b8dca922f3cef768dda07b53` | `REDALERT.MIX` |
| `WOLAPI.MIX` | 211145 | `aba7915e3c5c24ef95d55dccca402ee3bde1e6bdcc927e6398831328a63981a3` | `WOLAPI.MIX` |

Known Steam filenames are accepted only when both byte size and SHA-256 match this table. An otherwise structurally valid, unknown MIX can be imported under its uppercase filename, but it is not identified as the complete Steam layout. Destination collisions reject the transaction before the live asset directory is replaced.

## Installed receipt

The shared Apple importer writes `provenance.json` beside the imported asset tree in Application Support on macOS and iPadOS. That runtime receipt contains the detected source (`Steam` for a complete known set), source ID (`2229840`), import timestamp, and one entry per source file with its mapped path, size, and hash.

The installed receipt and imported payload are user data. Neither is copied into the app bundle, source tree, CI artifacts, or release packages.

## Safe replacement

The native Data settings never replace archives underneath a running game. A selected MIX set, folder, or ISO is validated into a sibling `.pending-game-data` transaction first. The active asset tree remains unchanged on cancellation, validation failure, or staging failure. At the next cold launch—before the engine opens an archive—the pending tree is activated atomically. RAtouch carries forward every non-MIX/ISO file from the active tree, including `redalert.ini`, saves, and local hidden state, while the new receipt and validated asset set replace the old imported payload.

An isolated real-file fixture stages the ten known Steam files, verifies the active tree remains untouched until activation, then proves that config, save, and hidden-state markers survive while a stale MIX does not. The iPad Game Data sheet exposes the current receipt summary, opens the replacement Files picker, and exports current `savegame.*` files through the system Files destination picker.
