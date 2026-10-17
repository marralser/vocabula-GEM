# Vocabula for GEM

**Vocabula for GEM** is a small, friendly vocabulary trainer for Digital
Research GEM and OpenGEM. It is written in ANSI C, builds with Open Watcom C,
and runs as the 16-bit DOS application `VOCABULA.APP`. Vocabula for GEM is derived 
from the DOS edition of Vocabula (https://github.com/marralser/Vocabula-DOS).

The current release is **Vocabula for GEM 0.1**.


GEM, "the Graphics Environment Manager" was a graphical desktop environment
created by Digital Research in the 1980s. It brought windows, icons, menus,
dialogs, mouse control, and graphical applications to DOS computers. Some 
consider it was ahead of its time.

Vocabula for GEM is a native GEM application. It uses GEM's window manager,
file selector, alerts, mouse input, and VDI graphics:
Vocabula for GEM is derived from the DOS edition of Vocabula. 
(https://github.com/marralser/Vocabula-DOS).
Vocabula for GEM retains the same lesson engine, answer classifications, scoring principles, error
repetition, statistics, and deliberately simple lesson format, but presents
them through a graphical GEM interface.

The DOS and GEM editions remain separate programs and can be developed or used
independently. Their `.VOC` lesson files are fully compatible: an existing DOS
lesson can be opened by the GEM edition without conversion, and a lesson
modified by the GEM edition can be used again by `VC.EXE`. The associated
plain-text statistics files are compatible as well. You may want to visit 
the Vocabula DOS repository https://github.com/marralser/Vocabula-DOS 
for downloading some .VOC lession files. 

## Why Vocabula?

Vocabulary training was one of the key educational uses of home computers in
the DOS era. Many vocabulary trainers were distributed as public-domain or
shareware programs, alongside a number of commercial packages.

I tried several of these programs at the time, but was never entirely happy
with them. The programs I knew tended to be buggy, overly complicated, or
missing functionality that matters during real practice. Some stored lessons
in cumbersome formats, making it unnecessarily difficult to add new words.
Others did not distinguish a keyboard typo or spelling mistake from an
incorrect translation.

Vocabula for DOS was created as a simpler alternative: a focused vocabulary trainer
that makes practice straightforward, responds helpfully to different kinds of
mistakes, and stores lessons in ordinary text files that teachers, parents,
and students can edit themselves. Vocabula for GEM intends to revive some of
the excitement that existed for this graphical shell at the time. However,
only limited numbers of Apps were developed for GEM at the time. Vocabula for GEM 
intends to show it could have gone otherwise.

## Features

- Native GEM application window with mouse and keyboard control.
- GEM file selector for choosing `.VOC` lessons.
- Practice from either language to the other, or mix both directions.
- Accept several translations or synonyms for each word and phrase.
- Distinguish exact answers, capitalization differences, apostrophe variants,
  adjacent-key typos, spelling mistakes, and incorrect translations.
- Recognize adjacent-key typos on QWERTY and QWERTZ keyboards.
- Count a keyboard typo as correct while displaying the expected spelling.
- Show the correct spelling and require retyping after a spelling mistake.
- Provide progressive hints and reveal an answer when needed.
- Require a revealed answer to be typed, helping the student remember it.
- Repeat incorrectly answered cards at the end of a lesson.
- Allow a genuinely correct alternative translation to be added to the lesson
  after explicit confirmation with the **Synonym** button.
- Keep scores, per-card history, detailed error records, and a ten-entry Hall
  of Fame containing date, time, student name, and score.
- Provide optional PC-speaker feedback.
- Offer optional case-sensitive answer checking without changing the lesson.
- Display lesson progress graphically.

## Lesson (`.VOC`) files

Lessons are plain-text files with the extension `.VOC`. No lesson editor,
database, conversion utility, or proprietary format is required.

The first meaningful line names the two languages:

```text
English : German
```

Every following line contains one vocabulary card. A colon separates the two
languages. Commas or forward slashes separate accepted synonyms:

```text
English : German
watching, looking : sehen, schauen
house/home : Haus/Zuhause
to play : spielen
```

To add a card, add another line. To accept another synonym, add it after a
comma or slash. Blank lines and lines beginning with `#` are ignored:

```text
# Words for lesson 4
the book : das Buch
teacher : Lehrer, Lehrerin
```

A backslash quotes punctuation that would otherwise act as a separator:

```text
comma\, word : Kommawort
path\/name : Pfadname
colon\: word : Doppelpunktwort
```

Spaces and periods are ignored during answer checking. For example, `New York`,
`NewYork`, and `New... York.` are treated as the same answer.

For best compatibility with DOS and GEM, use DOS line endings, code page 850
for accented characters, and an 8.3-compatible filename such as
`LESSON1.VOC`.

### Optional articles and infinitive markers

Common leading articles and infinitive markers may be included or omitted.
For example, Vocabula treats these as equivalent:

- `house` and `the house`
- `Haus` and `das Haus`
- `play` and `to play`
- `homme` and `l'homme`

The optional prefixes cover English, German, French, Italian, and Spanish:

```text
the, to, der, die, das, le, la, les, l', il, lo, i, gli, el, los, las
```

Only leading prefixes are optional. The same letters occurring inside a word
remain significant.

## Answer checking

| Answer type | Result |
| --- | --- |
| Correct | Counted as correct and rewarded with points |
| Capitalization difference | Counted as correct unless case-sensitive mode is enabled |
| Apostrophe-key difference (`'`, `` ` ``, or `Â´`) | Expected form shown; counted as correct |
| Adjacent-key typo | Correct spelling shown; counted as correct |
| One- or two-letter spelling mistake | Correct spelling shown; student retypes it |
| Incorrect translation | Student tries again; card is recorded as a mistake |

After three incorrect attempts, Vocabula displays the expected answer and asks
the student to type it. Incorrect cards can be repeated at the end of the
lesson.

## Controls

| Control | Action |
| --- | --- |
| `Enter` or **Enter** button | Submit an answer |
| `Backspace` | Edit the answer |
| `F1` or **Hint** | Show a progressively longer hint |
| `F2` or **Reveal** | Reveal the answer, which must then be typed |
| **Synonym** | Review and save a proposed alternative translation |
| `Esc` or **Stop** | Show the session summary and ask whether to stop |
| **Sound** checkbox | Enable or disable PC-speaker feedback |
| **Case sensitive** checkbox | Require capitalization to match |

## Files and statistics

Vocabula stores its plain-text statistics beside the selected lesson. For a
lesson named `VOC\LESSON.VOC`, it uses:

| File | Contents |
| --- | --- |
| `LESSON.SUM` | Completed-session summaries, names, and scores |
| `LESSON.CAR` | Per-card correct/incorrect history |
| `LESSON.ERR` | Detailed wrong-answer log |
| `LESSON.BAK` | Previous lesson version after saving a synonym |

Deleting a statistics file resets the corresponding history. The `.VOC` file
is changed only when the student explicitly confirms a new synonym.

Because these files use the same format as the DOS edition, both programs can
share the same lesson directory and learning history.

## Building

The supplied `MAKEFILE` targets Open Watcom C 1.9 in the 16-bit DOS large
memory model. It additionally requires the
[Open Watcom bindings for OpenGEM](https://github.com/BinaryMelodies/OpenGEM-Open-Watcom).

Extract the bindings as `GEMBIND` below the Vocabula source directory. Build
the required large-model library and GEM startup object:

```sh
make -C GEMBIND/libsrc/large
cp GEMBIND/libsrc/large/gembnd-l.lib GEMBIND/lib/gembnd-l.lib
make -C GEMBIND/samples main
```

Then build Vocabula:

```sh
wmake -f MAKEFILE
```

The result is:

```text
VCGEM.APP
```

Detailed instructions are provided in [`BUILDING.TXT`](BUILDING.TXT).

## Running

Copy `VCGEM.APP` and the `VOC` lesson directory to a location accessible from
the GEM desktop. Start GEM or OpenGEM, browse to the application, and open it.

Vocabula for GEM has been built and tested with Open Watcom C 1.9 and Digital
Research GEM 3.13. It is also intended for OpenGEM and compatible PC GEM
environments.

## Current limits

- Up to 500 cards per lesson.
- Up to 8 synonyms on either side of a card.
- Up to 79 bytes per individual word or phrase.
- Lesson lines are limited to 1023 bytes.
- Hall of Fame display is limited to the ten highest scores per lesson.
- The interface currently uses VDI drawing rather than an `.RSC` resource file.
- No GEM menu bar is currently required; the principal actions use buttons.

These limits keep the program practical on real-mode DOS computers with
conventional-memory constraints.

## Project background

The DOS edition of Vocabula began as a clean-room successor to the German DOS
vocabulary trainer Pauker. It does not use the original Pauker source code and
developed its own lesson format, answer checking, interface, and training
behavior. Vocabula for GEM subsequently brought that engine and file format to
the graphical GEM desktop.

## License

Vocabula is released under the MIT License. See `LICENSE.TXT` for the complete
license text.
