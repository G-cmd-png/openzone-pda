# Contributing to OpenZone

Thanks for wanting to help. A few things to know before you open a pull request.

## Licence and rights

OpenZone is licensed under **CC BY-NC-SA 4.0** with an additional permission (see
`LICENSE` and `NOTICE`).

By submitting a contribution you agree that:

1. The contribution is your own work, or you have the right to submit it.
2. You assign copyright in the contribution to the project owner, or — where your
   jurisdiction does not permit assignment — you grant the owner an irrevocable,
   worldwide, royalty-free licence to use, modify, sublicense and relicense it,
   including under terms different from CC BY-NC-SA 4.0.

Point 2 exists so the project can be relicensed later without hunting down every
past contributor. Without it, a single unreachable contributor can freeze the
licence forever.

Contributions that carry code under a licence incompatible with CC BY-NC-SA 4.0
cannot be accepted. **GPL code in particular cannot go in.**

## Ground rules for the mod repositories

- **Do not invent DayZ API.** Every engine call must be checked against the
  unpacked game scripts. If you are not sure, unpack the PBO and look.
- **No text in code.** Every user-facing string goes through the stringtable.
  `original` is Ukrainian, `english` is English, and the twelve remaining vanilla
  columns repeat `original`; the capital Ukrainian `І` is stored as the Latin `I`
  because no Metron font draws U+0406.
- **`OpenZone_PDA/gui/layouts/*.layout` and `OpenZone_PDA_VPP/gui/layouts/*.layout`
  files are generated, never hand-edited** -- with three named exceptions:
  `oz_pda_hud.layout`, `oz_pda_hud_edit.layout` and `oz_pda_hud_proxy.layout`
  (proportional layouts, hand-written on purpose, palette kept by hand). Every
  other layout comes from its mod's `ui/<mod>/*.json` descriptions; regenerate
  with the MCP's `layout_build` or `python -m dayz_mcp.layoutgen <root>` from
  the generator repo, both with no mod argument -- two mods carry descriptions
  now. Run the gallery at two sizes and in two languages before calling a UI
  change done.
- **`ui/tokens.json` lives in `openzone-core`, not in this repository.** This
  repo reads it through its own `[build] tokens`, and so does
  `openzone-factions` to lay out the faction page that shares the contacts tab
  (`$device.satellite`); changing a token there can re-lay a page in either
  repository.
- **Icons**: `tools/icons/make_icons.py` draws the atlas and writes the PNG and
  the `.imageset`; it does not touch `ui/OpenZone_PDA/oz_pda_icons_sheet.json`
  (the gallery sheet description), which is kept by hand and must list the same
  sprite names.
- **No hard dependency beyond Community Framework and OpenZone Core.** Those two are
  in `requiredAddons` by design — without the core the game refuses to load the PDA
  at all, and that is the platform decision of 2026-09-01, not an oversight.
  Everything else is an optional provider behind an `#ifdef` plus a runtime probe,
  with a working fallback; VPP in particular is reached only from the separate
  `OpenZone_PDA_VPP` pbo, which is why the PDA itself never mentions it.
- **Never identify an item by inheritance from our own class.** Item classnames come
  from JSON so that admins can point the mod at items from any mod.
- Every `.ps1` file must be saved as **UTF-8 with BOM**, or Windows PowerShell 5.1
  reads it in the system codepage and the parser dies on non-ASCII.

## Before you open a pull request

- The mod compiles: server boot and client compile check both clean.
- No new warnings in the server log.
- If you added a config field, it has a default, a migration, and a line in
  `Validate()`.
