# AMDNR 0.3.5.7 Public View — preparation status

## Completed

- Public launcher 0.3.5.35 signed with the existing AMDNR owner certificate and DigiCert timestamp. The actual launcher's AuthenticodeExeCheck returns Verified=true and the pinned owner thumbprint. Windows reports an untrusted root because the owner certificate is self-signed; no system trust settings were changed.
- Nine supplied README translations updated by adding a localized 0.3.5.7 section and changing the title. The supplied body, historical entries, links and attribution were preserved byte-for-byte. CHANGELOG.md has a new entry without rewriting history. Original Documents files were not modified.
- Worker deployed as ea34c35f-57a3-482f-9df1-2fc4b2535e15 after 778 passing tests, TypeScript checking, dry-run and comparison with the downloaded deployed script. Only the intended chat files and added retention module differ; other bundle modules are unchanged.
- Migration 0017 applied; minute cron active. Read-only remote query found zero expired public_chat rows, the public chat response contained no expired UTC messages, and both existing public file assets match local SHA-256 values.

## Publication boundaries

The public release manifest is unchanged: launcher 0.3.5.25, mod 0.3.5.6. GitHub and Discord were not published. The supplied ZIP contains the signed launcher and documentation, NOT a complete replacement mod/runtime package.

manifest.launcher-only.staged.json has the actual signed executable hash but references a planned GitHub URL that has not been uploaded. It retains mod 0.3.5.6. GitHub/manifest.public.template.json targets mod 0.3.5.7 but intentionally leaves the complete-package hash unresolved. Neither is activated.

## Remaining work for the complete mod release

The verified earlier mod delivery is a Supporter Preview artifact. Do not relabel it as a new public lmxxf 0.4.4/mochi 0.0.7 package. lmxxf still has opt-in qualification boundaries and mochi 0.0.7 still needs the correct live-resize/control test. Daniel temporal variation remains in the visual fixture. General in-game flicker, blur, crash and VRAM claims are not established.

The final public runtime build, package audit, final hash and corresponding source/provenance are still needed. No experimental candidate or confidential runtime was included in this preparation bundle.

## Backend recovery

Previous Worker version: c7edbc10-b274-4709-bbe9-dd273c7a7073. D1 pre-migration Time Travel bookmark is recorded in d1-before-bookmark.json. Code rollback does not reverse expired-message deletion. The migration implements the owner's requested rolling 48-hour retention; do not restore expired messages into the active chat.

Validation scripts, server evidence and private build paths are kept outside the distributable GitHub directory.
