open Brisk;

type attribute = [ Layout.style | Styles.viewStyle];

type style = list(attribute);

let component = nativeComponent("Image");

let measure = (node, _, _, _, _) => {
  open Layout.FlexLayout.LayoutSupport.LayoutTypes;

  let {context: img}: node = node;

  let width = BriskImage.getImageWidth(img) |> int_of_float;
  let height = BriskImage.getImageHeight(img) |> int_of_float;

  {width, height};
};

let make = (~style=[], ~source, children) =>
  component((_: Hooks.empty) =>
    {
      make: () => {
        let view = BriskImage.make(~source, ());
        {view, layoutNode: Layout.Node.make(~measure, ~style, view)};
      },
      configureInstance: (~isFirstRender as _, {view} as node) => {
        style
        |> List.iter(attribute =>
             switch (attribute) {
             | #Styles.viewStyle => Styles.setViewStyle(view, attribute)
             | #Layout.style => ()
             }
           );
        node;
      },
      children,
    }
  );

let createElement = (~style=[], ~source, ~children, ()) =>
  element(make(~style, ~source, listToElement(children)));
