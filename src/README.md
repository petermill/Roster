# Roster: locomotive roster manager (GTK3 / JMRI XML)

A small graphical program (C, GTK3, libxml2) to manage the **locomotive roster** used by the BiDiB system
GBM_CS / WiBiDiB2 and served to phones by **Engine Driver** (WiThrottle).

The roster is a `roster.xml` file in **JMRI format**: other tools can still read it, and the elements this program
does not know about (sound labels, groups, key/value pairs, ...) are kept untouched on every change.

## Features

- Loco list: name, address, protocol, manufacturer, model, date, **M4 UID**.
- **New**: adds a loco (refuses an empty or already used name).
- **Edit**: modifies the selected loco **in place**; everything that is not in the form (UID, `attributepairs`,
  `soundlabels`, images, ...) is preserved. The name can be changed (refused if it already exists).
- **Delete**: removes the selected loco.
- **Function** page: labels F0 to F28, with a "lockable" switch (latching or momentary function).
- **UID** field (Maerklin M4): 8 hexadecimal characters, stored in `<attributepairs>` under the key `M4_UID`.
  An empty field removes the key; an invalid value is refused with a message.
- **Import CS**: compares the Command Station database (`database.cur`) with the roster, shows a report **without
  writing anything**, then applies it if you click *Apply* (see below).
- **Aiguillages** page: the list of turnouts announced to Engine Driver, stored in `turnouts.xml` (see below).

## Dependencies

Debian / Ubuntu:

```
sudo apt install build-essential pkg-config libgtk-3-dev libxml2-dev
```

## Build

**Command line** (from the source folder):

```
gcc -fcommon -rdynamic *.c -o Roster \
    $(pkg-config --cflags --libs gtk+-3.0 libxml-2.0 gmodule-export-2.0)
```

**With Eclipse CDT**: mind the following points.

- Compiler: `pkg-config --cflags gtk+-3.0 libxml-2.0` and `-fcommon` in *Other flags*.
- Linker: `-rdynamic`, and the libraries one by one in *Libraries (-l)*:
  `gtk-3 gdk-3 z pangocairo-1.0 pango-1.0 harfbuzz atk-1.0 cairo-gobject cairo gdk_pixbuf-2.0 gio-2.0 gobject-2.0 xml2 gmodule-2.0 glib-2.0`.
  These settings are **per configuration** (Debug / Release): set them in the one you actually use.
- Without `-rdynamic`, GTK cannot find the `on_btn_*` handlers and the buttons do nothing, with no error message.
- `-fcommon` is needed as long as `GTK_pointer.h` defines variables instead of declaring them `extern`.
- Eclipse also runs `g++` for its automatic discovery: `sudo apt install g++`, or disable the provider
  *CDT GCC Built-in Compiler Settings* (the project is plain C).

## Running

The program opens `roster.glade` and `../../roster.xml` using **paths relative to the current directory**:
start it from a folder that contains `roster.glade` and where `../../roster.xml` exists (the working directory of
the run configuration in Eclipse). The roster path is the `roster` variable in `roster_main.c`;
`turnouts.xml` is looked for in the same folder as `roster.xml`.

## Roster format

One loco:

```xml
<locomotive id="FRET" dccAddress="8" mfg="Maerklin" model="" owner="" roadName="">
  <dateUpdated>2026-11-07</dateUpdated>
  <decoder model="Lokpilot V4"> </decoder>
  <protocol>M4</protocol>
  <functionlabels>
    <functionlabel num="0" lockable="true">Lights</functionlabel>
    <functionlabel num="2" lockable="false">Horn</functionlabel>
  </functionlabels>
  <attributepairs>
    <keyvaluepair>
      <key>M4_UID</key>
      <value>ffd4e3d2</value>
    </keyvaluepair>
  </attributepairs>
</locomotive>
```

Protocols: `M4`, `dcc_short`, `dcc_long` (long DCC addresses start at 128). The *Owner*, *Road name* and *Decoder*
fields are hidden in the user interface but kept in the file.

On the Command Station side, the loader `roster_xml_loader.c` reads: `id`, `dccAddress`, `mfg`, `model`, `roadName`,
`roadNumber`, `maxSpeed`, `<decoder>`, `<protocol>` (or `<locoaddress>`) and `<functionlabels>`. It **ignores**
`<attributepairs>` and `<soundlabels>`, so the M4 UID does not get in the way.

## Import from the Command Station

The **Import CS** button asks for the CS `database.cur` file, then shows a report before anything is written.

An M4 loco is identified by its **UID**; a DCC / MM loco by (protocol family, address).

| Situation                                              | Proposed action                                    |
|--------------------------------------------------------|----------------------------------------------------|
| UID already in the roster, same address                | `=` up to date (the roster name is never changed)  |
| UID present, different address                         | `~` address update                                 |
| M4 with the same name, no UID in the roster            | `+` link the UID (and the address)                 |
| Not in the roster                                      | `+` add (name, address, protocol, UID)             |
| Name already used with another UID, address already used, empty name in the CS | `!` **conflict**, never applied |

Locos that exist only in the roster are listed and never modified. The empty CS entry (index 0) is ignored.
Before writing, a copy `roster.xml.bak` is made; the file is then reformatted and the list refreshed.
A second import of the same file must show everything as "up to date".

Added locos have no manufacturer and no functions: fill them in afterwards. The protocol of Motorola formats is
written `motorola`: value **to be confirmed**, no MM loco has been tested.

## Turnouts (page *Aiguillages*)

The **Aiguillages** page of the main stack manages the turnouts that Engine Driver shows in its turnout list.
They are stored in a **separate file, `turnouts.xml`, in the same folder as `roster.xml`** (JMRI keeps its turnouts
in its own turnout table, not in the roster either).

```xml
<?xml version="1.0" encoding="UTF-8"?>
<turnouts>
  <turnout number="21" name="VI-1" comment="" inverted="false"/>
  <turnout number="22" name="VI-2" comment="entrance" inverted="true"/>
</turnouts>
```

| Field | Meaning |
|---|---|
| `number` | Turnout number as shown in Engine Driver: system name `LT<number>`. The address sent on the bus is `number - 1`. **Cannot be changed** once created (like the system name in JMRI): to renumber, add a new turnout and delete the old one. |
| `name` | User name, shown by Engine Driver (empty: Engine Driver shows `LT<number>`). The characters `" | { } [ ]` are refused. |
| `inverted` | Swaps *closed* and *thrown* for a turnout wired the other way round (same meaning as *Inverted* in the JMRI turnout table). |
| `comment` | Free note, **never sent** to the Pico. |

Rules: number from 1 to 2048 and unique, **32 turnouts at most**. The list is sorted by number.

- **Add** creates a turnout from the form (select a row first if you want to copy its values, then change the number).
- **Edit** modifies the selected turnout (name, inverted, comment).
- **Delete** asks for confirmation.
- Every change is **saved immediately** (written to a temporary file, then renamed, so the file is never left truncated),
  and a short message confirms it. If the write fails, the change is cancelled.

The Command Station reads `turnouts.xml` and sends it to the Pico right after the roster, in the same way (see the
WiBiDiB2 README). Without a `turnouts.xml`, nothing is sent and the Pico keeps its built-in table. The Pico does not
store the turnouts in flash: the Command Station sends them again at every subscription.

## Source layout

| File | Role |
|---|---|
| `roster_main.c`, `GTK_pointer.h` | GTK initialisation, loading of `roster.glade`, widget pointers |
| `load_roster.c/.h` | fills the list from `roster.xml` (including the UID column) |
| `manage_roster.c/.h` | New, Edit (in place), Delete, selection, UID |
| `load_function.c/.h`, `manage_function.c/.h` | function page F0 to F28 |
| `cs_import.c/.h` | `database.cur` parsing, plan, report, apply (no GTK) |
| `import_glue.c` | GTK part of the import: file chooser, report, backup |
| `turnouts_xml.c/.h` | reads / writes / checks `turnouts.xml` (libxml2 only, no GTK; shared with GBM_CS) |
| `turnouts_page.c/.h` | the *Aiguillages* page: list, form, Add / Edit / Delete, saving |
| `xmllint_pm.c`, `xmllint.h` | reformats `roster.xml` after writing |
| `roster.glade` | user interface (Glade) |

## Known limitations

- The roster reading code assumes a file like those written by JMRI (`<roster>` under the root element).
- `delete_node()` (Delete button) still walks the tree after freeing a node: to be rewritten.
- Turnouts: the number of a turnout cannot be edited, and the Roster program does not check that the numbers
  match what is wired on the layout.
- A loco in the original JMRI layout (protocol under `<locoaddress>`) modified with Edit also receives a direct
  `<protocol>`; harmless for the CS, but redundant.

## License

GNU General Public License v2 (see the source headers).
Partly derived from OpenDCC by Wolfgang Kufer. (c) Pierre Moulin.
