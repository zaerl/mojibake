// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use mojibake::bidi::{Direction, Paragraph, Run};
use mojibake::Error;

fn run(visual: std::ops::Range<usize>, logical: std::ops::Range<usize>, level: u8) -> Run {
    Run {
        visual,
        logical,
        level,
        direction: if level % 2 == 1 {
            Direction::Rtl
        } else {
            Direction::Ltr
        },
    }
}

#[test]
fn hebrew_in_latin() {
    for direction in [Direction::Ltr, Direction::Auto] {
        let paragraph = Paragraph::new("abc אבג", direction).unwrap();
        let line = 0..paragraph.len();

        assert_eq!(paragraph.direction(), Direction::Ltr);
        assert_eq!(paragraph.level(), 0);
        assert_eq!(paragraph.levels(), [0, 0, 0, 0, 1, 1, 1]);
        assert_eq!(
            paragraph.reorder_line(line.clone()).unwrap(),
            [0, 1, 2, 3, 6, 5, 4]
        );
        assert_eq!(
            paragraph.runs(line.clone()).unwrap(),
            [run(0..4, 0..4, 0), run(4..7, 4..7, 1)]
        );
        assert_eq!(paragraph.visual(line).unwrap(), "abc גבא");
    }
}

#[test]
fn latin_in_hebrew() {
    // The first strong character is Hebrew, so the paragraph is right to left.
    let paragraph = Paragraph::new("שלום world!", Direction::Auto).unwrap();
    let line = 0..paragraph.len();

    assert_eq!(paragraph.direction(), Direction::Rtl);
    assert_eq!(paragraph.levels(), [1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1]);
    assert_eq!(
        paragraph.runs(line.clone()).unwrap(),
        [
            run(0..1, 10..11, 1),
            run(1..6, 5..10, 2),
            run(6..11, 0..5, 1)
        ]
    );
    assert_eq!(paragraph.visual(line).unwrap(), "!world םולש");

    // A line is reordered on its own: the first word alone.
    assert_eq!(paragraph.reorder_line(0..4).unwrap(), [3, 2, 1, 0]);
    assert_eq!(paragraph.visual(0..4).unwrap(), "םולש");
}

#[test]
fn arabic_with_numbers_and_mirroring() {
    let text = "العربية 123 (abc)";
    let paragraph = Paragraph::new(text, Direction::Rtl).unwrap();
    let line = 0..paragraph.len();

    assert_eq!(
        paragraph.levels(),
        [1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 2, 2, 2, 1]
    );
    assert_eq!(
        paragraph.runs(line.clone()).unwrap(),
        [
            run(0..1, 16..17, 1),
            run(1..4, 13..16, 2),
            run(4..6, 11..13, 1),
            run(6..9, 8..11, 2),
            run(9..17, 0..8, 1),
        ]
    );

    // The parentheses resolve to the right-to-left level and are mirrored (rule L4).
    let mirrored: Vec<(char, char)> = paragraph
        .chars()
        .filter_map(|c| c.mirror.map(|mirror| (c.ch, mirror)))
        .collect();
    assert_eq!(mirrored, [('(', ')'), (')', '(')]);
    assert_eq!(paragraph.visual(line).unwrap(), "(abc) 123 ةيبرعلا");

    // Positions map back to the text.
    let chars: Vec<_> = paragraph.chars().collect();
    assert_eq!(chars[8].ch, '1');
    assert_eq!(&text[paragraph.byte_range(8..11).unwrap()], "123");
    assert_eq!(&text[paragraph.byte_range(0..7).unwrap()], "العربية");
}

#[test]
fn removed_controls() {
    // RLE ... PDF are removed by rule X9, so the paragraph has three characters.
    let text = "a\u{202b}b\u{202c}c";
    let paragraph = Paragraph::new(text, Direction::Ltr).unwrap();

    assert_eq!(paragraph.len(), 3);
    assert_eq!(
        paragraph.chars().map(|c| c.byte_offset).collect::<Vec<_>>(),
        [0, 4, 8]
    );
    // `b` is L at embedding level 1, so rule I2 raises it to 2.
    assert_eq!(paragraph.levels(), [0, 2, 0]);
    assert_eq!(&text[paragraph.byte_range(1..2).unwrap()], "b\u{202c}");
}

#[test]
fn empty_and_invalid_ranges() {
    let empty = Paragraph::new("", Direction::Auto).unwrap();

    assert!(empty.is_empty());
    assert_eq!(empty.direction(), Direction::Ltr);
    assert_eq!(empty.reorder_line(0..0).unwrap(), Vec::<usize>::new());
    assert_eq!(empty.runs(0..0).unwrap(), []);
    assert_eq!(empty.visual(0..0).unwrap(), "");

    let paragraph = Paragraph::new("abc", Direction::Rtl).unwrap();

    assert_eq!(paragraph.direction(), Direction::Rtl);
    assert_eq!(paragraph.reorder_line(1..1).unwrap(), Vec::<usize>::new());
    assert_eq!(paragraph.reorder_line(0..4), Err(Error::InvalidArgument));
    #[allow(clippy::reversed_empty_ranges)]
    let reversed = 2..1;
    assert_eq!(paragraph.runs(reversed), Err(Error::InvalidArgument));
    assert_eq!(paragraph.byte_range(0..4), Err(Error::InvalidArgument));
}

#[test]
fn paragraph_is_send_and_sync() {
    fn assert_send_sync<T: Send + Sync>() {}

    assert_send_sync::<Paragraph<'static>>();
}
