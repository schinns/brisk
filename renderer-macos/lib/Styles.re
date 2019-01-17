open Layout;
open BriskStylableText;

type textStyle = [
  | `Color(Color.t)
  | `Font(Font.t)
  | `Kern(float)
  | `Align(Alignment.t)
  | `LineBreak(LineBreak.t)
  | `LineSpacing(float)
];

let flushTextStyle = BriskStylableText.applyChanges;

let setTextStyle = (txt: text, attr: [> textStyle]) =>
  switch (attr) {
  | `Font(({family, size, weight}: Font.t)) =>
    let weight =
      switch (weight) {
      | `UltraLight => (-0.8)
      | `Thin => (-0.6)
      | `Light => (-0.4)
      | `Regular => 0.
      | `Medium => 0.23
      | `Semibold => 0.3
      | `Bold => 0.4
      | `Heavy => 0.56
      | `Black => 0.62
      };
    setFont(txt, family, size, weight);
  | `Kern(kern) => setKern(txt, kern)
  | `Align(align) =>
    setAlignment(
      txt,
      switch (align) {
      | `Left => 0
      | `Right => 1
      | `Center => 2
      | `Justified => 3
      | `Natural => 4
      },
    )
  | `LineBreak(mode) =>
    setLineBreak(
      txt,
      switch (mode) {
      | `WordWrap => 0
      | `CharWrap => 1
      | `Clip => 2
      | `TruncateHead => 3
      | `TruncateTale => 4
      | `TruncateMiddle => 5
      },
    )
  | `LineSpacing(spacing) => setLineSpacing(txt, spacing)
  | `Color(({r, g, b, a}: Color.t)) => setColor(txt, r, g, b, a)
  | _ => ()
  };
