# OpenZone PDA

A S.T.A.L.K.E.R.-style PDA for DayZ. One device you carry that holds the map, your
faction, your contacts, your notes and your conversations — and takes modules that
give it a GPS receiver, a decryptor, a spy plate, or a radio from another mod.

Built on [OpenZone Core](https://github.com/covalschi/openzone-core). Designed to run
on **any server and any map**, configured entirely from JSON.

## Planned features

- **Device profiles.** Admins declare any number of PDA classnames in JSON — including
  items from other mods — and give each its own pages, marker limits, radio range and
  power draw. A rookie's PDA is not a Duty officer's PDA.
- **Map and markers.** Personal, faction and server-wide markers; share a marker with a
  friend, your faction, or a radio channel.
- **Factions, contacts and friends.** Self-contained, with optional providers that pick
  up Expansion factions or parties when those are installed.
- **Chat backed by Discord.** Private conversations and group chats live as private
  Discord threads, visible only to their participants. Accounts are linked by Discord
  OAuth against the player's SteamID.
- **Configurable radio.** Virtual frequencies, per-model bands, interference, battery
  drain and a rebindable push-to-talk key.
- **Always-on overlay.** Chat and radio stay readable while you move — the full device
  screen is a separate menu.

## Requirements

- [Community Framework](https://steamcommunity.com/sharedfiles/filedetails/?id=1559212036)
- OpenZone Core

The Discord bridge is optional. Without it the PDA works; the chat pages report that
they are offline.

## Status

Early development. Nothing is published to the Workshop yet, and the items are not in
the central economy — an admin has to spawn them.

Working and exercised on a test server: the device itself (battery, power, PIN, lazy
auto-lock, factory reset), sealed quest devices opened with a decryptor on a
countdown, the seven-page shell with a tab strip built from the device's own profile,
per-device map markers and routes, data carriers (write, read, take one record,
erase) with shared markers and notes moving between devices on them, the contacts
list with the exchange done in the world by an action, notes, chat and the news feed
through the Discord bridge, an NPC pager, the transponder with its privacy modes,
the capsule a stolen device becomes, the HUD overlay with its movable panes, the
admin console panel in the VPP window, tiered battery drain (the profile's own
consumption per minute times the power factor of every board that works), and the
hardware contract that lets another mod add its own module, tab and page.

Built but not yet connected, so honestly: the quest journal (it draws, and waits
for a quest mod to fill it). The Geiger counter and the dosimeter were removed on
2026-09-09 — radiation is not this mod's subject, and a mod that wants it declares
its own board and its own page, the way the radio does.
Discord account linking lives in OpenZone Core and works; the
PDA only shows whether the account is linked. The list under **Planned features**
describes where this is going, not what it does today.

## Configuration

Everything an admin turns lives under `$profile:OpenZone\` and is written back
whenever the mod had to repair it.

- **`OZ_PDA_Profiles.json`** — one entry per PDA classname: its pages, its bays,
  its marker and memory limits, its power draw, its PIN length.
- **`OZ_PDA_Hardware.json`** — the modules and data carriers the device accepts.
  A module entry is a classname, a `Kind` (`gps`, `decryptor`, `spy`, or a word
  another mod brought), a `PowerFactor`, and
  `RangeM`. `RangeM` belongs to the **GPS** entry: the GPS receiver is also the
  transponder, so the range it declares is how far the device hears other
  beacons and is heard by them. Left at zero, it becomes 5000 m.
- **`OZ_PDA_Tuning.json`** — the numbers: text limits, PIN thresholds, radii,
  seconds. One of them is not a number:
  - **`MapHome`** — `"x z"`, where the map opens on a device with no GPS. A PDA
    without a GPS receiver does not know where it is, so the map cannot open on
    the player; it opens here instead, the same place every time. Left empty, it
    becomes the centre of the world the server is running (7680 7680 on
    Chernarus), and the file is rewritten with that point so it can be edited.

## Licence

CC BY-NC-SA 4.0 with an additional permission for server operators — see `LICENSE`
and `NOTICE`.
