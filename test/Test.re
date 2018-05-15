open Assert;

module Run = {
  type updateState = {
    previousReactElement: ReasonReact.reactElement,
    oldRenderedElement: ReasonReact.renderedElement,
    nextReactElement: ReasonReact.reactElement
  };
  type testItem('a) =
    | FirstRender(ReasonReact.reactElement): testItem(TestRenderer.t)
    | Update(updateState): testItem(
                             (
                               TestRenderer.t,
                               option(TestRenderer.testTopLevelUpdateLog)
                             )
                           )
    | FlushUpdates(ReasonReact.reactElement, ReasonReact.renderedElement): testItem(
                                                                    option(
                                                                    (
                                                                    TestRenderer.t,
                                                                    list(
                                                                    TestRenderer.testUpdateEntry
                                                                    )
                                                                    )
                                                                    )
                                                                    );
  let expect:
    type a.
      (~label: string=?, a, testItem(a)) =>
      (ReasonReact.RenderedElement.t, ReasonReact.reactElement) =
    (~label=?, expected, prev) =>
      switch prev {
      | Update({nextReactElement, oldRenderedElement, previousReactElement}) =>
        let (newRenderedElement, _) as actual =
          ReasonReact.RenderedElement.update(
            ~previousReactElement,
            ~renderedElement=oldRenderedElement,
            nextReactElement
          );
        assertUpdate(~label?, expected, actual);
        (newRenderedElement, nextReactElement);
      | FirstRender(previousReactElement) =>
        open TestRenderer;
        let oldRenderedElement = render(previousReactElement);
        assertElement(~label?, expected, oldRenderedElement);
        (oldRenderedElement, previousReactElement);
      | FlushUpdates(previousReactElement, oldRenderedElement) =>
        let (newRenderedElement, _) as actual =
          ReasonReact.RenderedElement.flushPendingUpdates(oldRenderedElement);
        switch expected {
        | Some(expected) =>
          assertFlushUpdate(~label?, expected, actual);
          (newRenderedElement, previousReactElement);
        | None =>
          check(
            Alcotest.bool,
            switch label {
            | None => "It is memoized"
            | Some(x) => x
            },
            oldRenderedElement === newRenderedElement,
            true
          );
          (newRenderedElement, previousReactElement);
        };
      };
  let start = reactElement => {
    ReasonReact.GlobalState.reset();
    FirstRender(reactElement);
  };
  let act = (~action, rAction, (oldRenderedElement, previousReactElement)) => {
    ReasonReact.RemoteAction.act(rAction, ~action);
    (oldRenderedElement, previousReactElement);
  };
  let update = (nextReactElement, (oldRenderedElement, previousReactElement)) =>
    Update({nextReactElement, oldRenderedElement, previousReactElement});
  let flushPendingUpdates = ((oldRenderedElement, previousReactElement)) =>
    FlushUpdates(previousReactElement, oldRenderedElement);
  let done_ = ((_oldRenderedElement, _previousReactElement)) => ();
};

let suite =
  Components.[
    (
      "Box wrapper",
      `Quick,
      () => {
        open Run;
        let twoBoxes =
          TestComponents.(
            <Div id=2>
              <Box id=4 state="ImABox" />
              <Box id=5 state="ImABox" />
            </Div>
          );
        let oneBox = TestComponents.(<Div id=2> <Box id=3 /> </Div>);
        start(<Components.BoxWrapper />)
        |> expect(
             TestComponents.[
               <BoxWrapper id=1>
                 <Div id=2> <Box id=3 state="ImABox" /> </Div>
               </BoxWrapper>
             ]
           )
        |> update(<Components.BoxWrapper twoBoxes=true />)
        |> expect((
             [TestComponents.(<BoxWrapper id=1> twoBoxes </BoxWrapper>)],
             Some({
               TestRenderer.subtreeChange: `Nested,
               updateLog:
                 ref(
                   TestComponents.[
                     TestRenderer.UpdateInstance({
                       stateChanged: false,
                       subTreeChanged:
                         `ReplaceElements((
                           [<Box id=3 state="ImABox" />],
                           [
                             <Box id=4 state="ImABox" />,
                             <Box id=5 state="ImABox" />
                           ]
                         )),
                       newInstance: twoBoxes,
                       oldInstance: oneBox
                     }),
                     UpdateInstance({
                       stateChanged: false,
                       subTreeChanged: `Nested,
                       newInstance:
                         <TestComponents.BoxWrapper id=1>
                           twoBoxes
                         </TestComponents.BoxWrapper>,
                       oldInstance: <BoxWrapper id=1> oneBox </BoxWrapper>
                     })
                   ]
                 )
             })
           ))
        |> done_;
      }
    ),
    (
      "Change counter test",
      `Quick,
      () => {
        open ReasonReact;
        open Run;
        let (beforeUpdate4, _) as testContinuation =
          start(<ChangeCounter label="defaultText" />)
          |> expect([
               TestComponents.(
                 <ChangeCounter id=1 label="defaultText" counter=10 />
               )
             ])
          |> update(<ChangeCounter label="defaultText" />)
          |> expect((
               [
                 TestComponents.(
                   <ChangeCounter id=1 label="defaultText" counter=10 />
                 )
               ],
               None
             ))
          |> update(<ChangeCounter label="updatedText" />)
          |> expect(
               TestComponents.(
                 [<ChangeCounter id=1 label="updatedText" counter=11 />],
                 Some(
                   TestRenderer.{
                     subtreeChange: `Nested,
                     updateLog:
                       ref([
                         UpdateInstance({
                           stateChanged: true,
                           subTreeChanged: `NoChange,
                           oldInstance:
                             <ChangeCounter
                               id=1
                               label="defaultText"
                               counter=10
                             />,
                           newInstance:
                             <ChangeCounter
                               id=1
                               label="updatedText"
                               counter=11
                             />
                         })
                       ])
                   }
                 )
               )
             )
          |> flushPendingUpdates
          |> expect(
               Some(
                 TestComponents.(
                   [<ChangeCounter id=1 label="updatedText" counter=2011 />],
                   [
                     TestRenderer.UpdateInstance({
                       stateChanged: true,
                       subTreeChanged: `NoChange,
                       oldInstance:
                         <ChangeCounter id=1 label="updatedText" counter=11 />,
                       newInstance:
                         <ChangeCounter
                           id=1
                           label="updatedText"
                           counter=2011
                         />
                     })
                   ]
                 )
               )
             )
          |> flushPendingUpdates
          |> expect(None)
          |> flushPendingUpdates
          |> expect(None)
          |> update(<ButtonWrapperWrapper wrappedText="updatedText" />)
          |> expect(
               ~label=
                 "Updating components: ChangeCounter to ButtonWrapperWrapper",
               TestComponents.(
                 [
                   <ButtonWrapperWrapper
                     id=2
                     nestedText="wrappedText:updatedText"
                   />
                 ],
                 Some(
                   TestRenderer.{
                     subtreeChange: `Nested,
                     updateLog:
                       ref([
                         ChangeComponent({
                           oldSubtree: [],
                           newSubtree: [
                             <Div id=3>
                               <Text id=4 title="buttonWrapperWrapperState" />
                               <Text id=5 title="wrappedText:updatedText" />
                               <ButtonWrapper id=6 />
                             </Div>
                           ],
                           oldInstance:
                             <ChangeCounter
                               id=1
                               label="updatedText"
                               counter=2011
                             />,
                           newInstance:
                             <ButtonWrapperWrapper
                               id=2
                               nestedText="wrappedText:updatedText"
                             />
                         })
                       ])
                   }
                 )
               )
             );
        let (afterUpdate, _) =
          testContinuation
          |> update(
               <ButtonWrapperWrapper wrappedText="updatedTextmodified" />
             )
          |> expect(
               ~label="Updating text in the button wrapper",
               TestComponents.(
                 [
                   <ButtonWrapperWrapper
                     id=2
                     nestedText="wrappedText:updatedTextmodified"
                   />
                 ],
                 Some(
                   TestRenderer.{
                     subtreeChange: `Nested,
                     updateLog:
                       ref([
                         UpdateInstance({
                           stateChanged: true,
                           subTreeChanged: `ContentChanged(`NoChange),
                           oldInstance:
                             <Text id=5 title="wrappedText:updatedText" />,
                           newInstance:
                             <Text
                               id=5
                               title="wrappedText:updatedTextmodified"
                             />
                         }),
                         UpdateInstance({
                           stateChanged: false,
                           subTreeChanged: `Nested,
                           oldInstance:
                             <Div id=3>
                               <Text id=4 title="buttonWrapperWrapperState" />
                               <Text id=5 title="wrappedText:updatedText" />
                               <ButtonWrapper id=6 />
                             </Div>,
                           newInstance:
                             <Div id=3>
                               <Text id=4 title="buttonWrapperWrapperState" />
                               <Text
                                 id=5
                                 title="wrappedText:updatedTextmodified"
                               />
                               <ButtonWrapper id=6 />
                             </Div>
                         }),
                         UpdateInstance({
                           stateChanged: false,
                           subTreeChanged: `Nested,
                           oldInstance:
                             <ButtonWrapperWrapper
                               id=2
                               nestedText="wrappedText:updatedText"
                             />,
                           newInstance:
                             <ButtonWrapperWrapper
                               id=2
                               nestedText="wrappedText:updatedTextmodified"
                             />
                         })
                       ])
                   }
                 )
               )
             );
        check(
          Alcotest.bool,
          "Memoized nested button wrapper",
          true,
          ReasonReact.(
            switch (beforeUpdate4, afterUpdate) {
            | (
                IFlat(
                  Instance({
                    instanceSubTree:
                      IFlat(
                        Instance({
                          instanceSubTree: INested(_, [_, _, IFlat(x)])
                        })
                      )
                  })
                ),
                IFlat(
                  Instance({
                    instanceSubTree:
                      IFlat(
                        Instance({
                          instanceSubTree: INested(_, [_, _, IFlat(y)])
                        })
                      )
                  })
                )
              ) =>
              x === y
            | _ => false
            }
          )
        );
      }
    ),
    (
      "Test Lists With Dynamic Keys",
      `Quick,
      () => {
        open ReasonReact;
        open TestComponents;
        open TestRenderer;
        open Run;
        let rAction = RemoteAction.create();
        start(<Components.BoxList useDynamicKeys=true rAction />)
        |> expect(~label="Initial BoxList", [<BoxList id=1 />])
        |> act(~action=Components.BoxList.Create("Hello"), rAction)
        |> flushPendingUpdates
        |> expect(
             ~label="Add Hello then Flush",
             Some((
               [
                 <BoxList id=1>
                   <BoxWithDynamicKeys id=2 state="Hello" />
                 </BoxList>
               ],
               [
                 UpdateInstance({
                   stateChanged: true,
                   subTreeChanged:
                     `ReplaceElements((
                       [],
                       [<BoxWithDynamicKeys id=2 state="Hello" />]
                     )),
                   oldInstance: <BoxList id=1 />,
                   newInstance:
                     <BoxList id=1>
                       <BoxWithDynamicKeys id=2 state="Hello" />
                     </BoxList>
                 })
               ]
             ))
           )
        |> act(~action=Components.BoxList.Create("World"), rAction)
        |> flushPendingUpdates
        |> expect(
             ~label="Add Hello then Flush",
             Some((
               [
                 <BoxList id=1>
                   <BoxWithDynamicKeys id=3 state="World" />
                   <BoxWithDynamicKeys id=2 state="Hello" />
                 </BoxList>
               ],
               [
                 UpdateInstance({
                   stateChanged: true,
                   subTreeChanged:
                     `ReplaceElements((
                       [<BoxWithDynamicKeys id=2 state="Hello" />],
                       [
                         <BoxWithDynamicKeys id=3 state="World" />,
                         <BoxWithDynamicKeys id=2 state="Hello" />
                       ]
                     )),
                   oldInstance:
                     <BoxList id=1>
                       <BoxWithDynamicKeys id=2 state="Hello" />
                     </BoxList>,
                   newInstance:
                     <BoxList id=1>
                       <BoxWithDynamicKeys id=3 state="World" />
                       <BoxWithDynamicKeys id=2 state="Hello" />
                     </BoxList>
                 })
               ]
             ))
           )
        |> act(~action=Components.BoxList.Reverse, rAction)
        |> flushPendingUpdates
        |> expect(
             ~label="Add Hello then Flush",
             Some((
               [
                 <BoxList id=1>
                   <BoxWithDynamicKeys id=2 state="Hello" />
                   <BoxWithDynamicKeys id=3 state="World" />
                 </BoxList>
               ],
               [
                 UpdateInstance({
                   stateChanged: true,
                   subTreeChanged: `Reordered,
                   oldInstance:
                     <BoxList id=1>
                       <BoxWithDynamicKeys id=3 state="World" />
                       <BoxWithDynamicKeys id=2 state="Hello" />
                     </BoxList>,
                   newInstance:
                     <BoxList id=1>
                       <BoxWithDynamicKeys id=2 state="Hello" />
                       <BoxWithDynamicKeys id=3 state="World" />
                     </BoxList>
                 })
               ]
             ))
           )
        |> done_;
      }
    ),
    (
      "Test Lists Without Dynamic Keys",
      `Quick,
      () => {
        open ReasonReact;
        open TestComponents;
        open TestRenderer;
        open Run;
        let rAction = RemoteAction.create();
        start(<Components.BoxList rAction />)
        |> expect(~label="Initial BoxList", [<BoxList id=1 />])
        |> act(~action=Components.BoxList.Create("Hello"), rAction)
        |> flushPendingUpdates
        |> expect(
          ~label="Add Hello then Flush",
          Some((
            [<BoxList id=1> <Box id=2 state="Hello" /> </BoxList>],
            [
              UpdateInstance({
                stateChanged: true,
                subTreeChanged:
                  `ReplaceElements(([], [<Box id=2 state="Hello" />])),
                oldInstance: <BoxList id=1 />,
                newInstance:
                  <BoxList id=1> <Box id=2 state="Hello" /> </BoxList>
              })
            ]
          ))
        )
        |> act(~action=Components.BoxList.Create("World"), rAction)
        |> flushPendingUpdates
        |> expect(
          ~label="Add Hello then Flush",
          Some((
            [
              <BoxList id=1>
                <Box id=3 state="World" />
                <Box id=4 state="Hello" />
              </BoxList>
            ],
            [
              UpdateInstance({
                stateChanged: true,
                subTreeChanged:
                  `ReplaceElements((
                    [<Box id=2 state="Hello" />],
                    [<Box id=3 state="World" />, <Box id=4 state="Hello" />]
                  )),
                oldInstance:
                  <BoxList id=1> <Box id=2 state="Hello" /> </BoxList>,
                newInstance:
                  <BoxList id=1>
                    <Box id=3 state="World" />
                    <Box id=4 state="Hello" />
                  </BoxList>
              })
            ]
          ))
        )
        |> act(~action=Components.BoxList.Reverse, rAction)
        |> flushPendingUpdates
        |> expect(
          ~label="Add Hello then Flush",
          Some((
            [
              <BoxList id=1>
                <Box id=3 state="Hello" />
                <Box id=4 state="World" />
              </BoxList>
            ],
            [
              UpdateInstance({
                stateChanged: true,
                subTreeChanged: `ContentChanged(`NoChange),
                oldInstance: <Box id=4 state="Hello" />,
                newInstance: <Box id=4 state="World" />
              }),
              UpdateInstance({
                stateChanged: true,
                subTreeChanged: `ContentChanged(`NoChange),
                oldInstance: <Box id=3 state="World" />,
                newInstance: <Box id=3 state="Hello" />
              }),
              UpdateInstance({
                stateChanged: true,
                subTreeChanged: `Nested,
                oldInstance:
                  <BoxList id=1>
                    <Box id=3 state="World" />
                    <Box id=4 state="Hello" />
                  </BoxList>,
                newInstance:
                  <BoxList id=1>
                    <Box id=3 state="Hello" />
                    <Box id=4 state="World" />
                  </BoxList>
              })
            ]
          ))
        )
        |> done_;
      }
    ),
    (
      "Deep Move Box With Dynamic Keys",
      `Quick,
      () => {
        open ReasonReact;
        GlobalState.reset();
        let box_ = <BoxWithDynamicKeys title="box to move" />;
        let rendered0 = RenderedElement.render(box_);
        TestRenderer.convertElement(rendered0)
        |> check(
             renderedElement,
             "Initial Box",
             [TestComponents.(<BoxWithDynamicKeys id=1 state="box to move" />)]
           );
        let updatedReactElement =
          Nested("div", [stringToElement("before"), Nested("div", [box_])]);
        let (rendered1, _) as actual1 =
          RenderedElement.update(
            ~previousReactElement=box_,
            ~renderedElement=rendered0,
            updatedReactElement
          );
        assertUpdate(
          ~label="After update",
          TestComponents.(
            [
              <Text id=2 title="before" />,
              <BoxWithDynamicKeys id=1 state="box to move" />
            ],
            Some(
              TestRenderer.{
                subtreeChange:
                  `ReplaceElements((
                    [<BoxWithDynamicKeys id=1 state="box to move" />],
                    [
                      <Text id=2 title="before" />,
                      <BoxWithDynamicKeys id=1 state="box to move" />
                    ]
                  )),
                updateLog: ref([])
              }
            )
          ),
          actual1
        );
        /* TODO Compare rendered0 and rendered1 */
        /* check(
             Alcotest.bool,
             "Memoized nested box",
             true,
             ReasonReact.(
               switch (rendered0, rendered1) {
               | (
                   IFlat([Instance({instanceSubTree: IFlat([x])})]),
                   IFlat([
                     Instance({instanceSubTree: INested(_, [_, IFlat([y])])})
                   ])
                 ) =>
                 x === y
               | _ => false
               }
             )
           ); */
      }
    ),
    (
      "Test With Static Keys",
      `Quick,
      () => {
        open ReasonReact;
        GlobalState.reset();
        let key1 = Key.create();
        let key2 = Key.create();
        let previousReactElement =
          ReasonReact.listToElement([
            <Box key=key1 title="Box1unchanged" />,
            <Box key=key2 title="Box2unchanged" />
          ]);
        let rendered0 = RenderedElement.render(previousReactElement);
        TestRenderer.convertElement(rendered0)
        |> check(
             renderedElement,
             "Initial Boxes",
             TestComponents.[
               <Box id=key1 state="Box1unchanged" />,
               <Box id=key2 state="Box2unchanged" />
             ]
           );
        let updatedReactElement =
          ReasonReact.listToElement([
            <Box key=key2 title="Box2changed" />,
            <Box key=key1 title="Box1changed" />
          ]);
        let previousReactElement = updatedReactElement;
        let (rendered1, _) as actual1 =
          RenderedElement.update(
            ~previousReactElement,
            ~renderedElement=rendered0,
            updatedReactElement
          );
        assertUpdate(
          ~label="Swap Boxes",
          TestComponents.(
            [
              <Box id=key2 state="Box2changed" />,
              <Box id=key1 state="Box1changed" />
            ],
            Some(
              TestRenderer.{
                subtreeChange: `Nested,
                updateLog:
                  ref([
                    UpdateInstance({
                      stateChanged: true,
                      subTreeChanged: `ContentChanged(`NoChange),
                      oldInstance: <Box id=1 state="Box1unchanged" />,
                      newInstance: <Box id=1 state="Box1changed" />
                    }),
                    UpdateInstance({
                      stateChanged: true,
                      subTreeChanged: `ContentChanged(`NoChange),
                      oldInstance: <Box id=2 state="Box2unchanged" />,
                      newInstance: <Box id=2 state="Box2changed" />
                    })
                  ])
              }
            )
          ),
          actual1
        );
      }
    ),
    (
      "Test Update on Alternate Clicks",
      `Quick,
      () => {
        open ReasonReact;
        GlobalState.reset();
        let result = (~state, ~text) => [
          {
            TestRenderer.id: 1,
            component: Component(UpdateAlternateClicks.component),
            state,
            subtree: [TestComponents.(<Text id=2 title=text />)]
          }
        ];
        let rAction = RemoteAction.create();
        let rendered =
          RenderedElement.render(<UpdateAlternateClicks rAction />);
        TestRenderer.convertElement(rendered)
        |> check(renderedElement, "Initial", result(~state="0", ~text="0"));
        RemoteAction.act(rAction, ~action=Click);
        let (rendered, _) = RenderedElement.flushPendingUpdates(rendered);
        check(
          renderedElement,
          "First click then flush",
          result(~state="1", ~text="0"),
          TestRenderer.convertElement(rendered)
        );
        RemoteAction.act(rAction, ~action=Click);
        let (rendered, _) = RenderedElement.flushPendingUpdates(rendered);
        check(
          renderedElement,
          "Second click then flush",
          result(~state="2", ~text="2"),
          TestRenderer.convertElement(rendered)
        );
        RemoteAction.act(rAction, ~action=Click);
        let (rendered, _) = RenderedElement.flushPendingUpdates(rendered);
        check(
          renderedElement,
          "Second click then flush",
          result(~state="3", ~text="2"),
          TestRenderer.convertElement(rendered)
        );
        RemoteAction.act(rAction, ~action=Click);
        let (rendered, _) = RenderedElement.flushPendingUpdates(rendered);
        check(
          renderedElement,
          "Second click then flush",
          result(~state="4", ~text="4"),
          TestRenderer.convertElement(rendered)
        );
      }
    ),
    (
      "Test flat update",
      `Quick,
      () => {
        open ReasonReact;
        GlobalState.reset();
        let previousReactElement = <Text key=1 title="x" />;
        let rendered = RenderedElement.render(previousReactElement);
        let updatedReactElement = <Text key=2 title="y" />;
        let actual =
          RenderedElement.update(
            ~previousReactElement,
            ~renderedElement=rendered,
            updatedReactElement
          );
        TestComponents.(
          assertUpdate(
            ~label="Will return `ReplaceElements for top level flat update",
            (
              [<Text id=2 title="y" />],
              Some({
                subtreeChange:
                  `ReplaceElements((
                    [<Text id=1 title="x" />],
                    [<Text id=2 title="y" />]
                  )),
                updateLog: ref([])
              })
            ),
            actual
          )
        );
      }
    ),
    (
      "Test no change",
      `Quick,
      () => {
        open ReasonReact;
        open TestComponents;
        GlobalState.reset();
        let key1 = Key.create();
        let key2 = Key.create();
        let previousReactElement =
          listToElement(
            Components.[
              <Text key=key1 title="x" />,
              <Text key=key2 title="y" />
            ]
          );
        let rendered0 = RenderedElement.render(previousReactElement);
        Assert.assertElement(
          [<Text id=1 title="x" />, <Text id=2 title="y" />],
          rendered0
        );
        let updatedReactElement =
          listToElement(
            Components.[
              <Text key=key1 title="x" />,
              <Text key=key2 title="y" />
            ]
          );
        let actual =
          RenderedElement.update(
            ~previousReactElement,
            ~renderedElement=rendered0,
            updatedReactElement
          );
        let previousReactElement = updatedReactElement;
        assertUpdate(
          ~label=
            "Updates the state because the state is updated with a new instance of string",
          (
            [<Text id=1 title="x" />, <Text id=2 title="y" />],
            Some({
              subtreeChange: `Nested,
              updateLog:
                ref(
                  TestRenderer.[
                    UpdateInstance({
                      stateChanged: true,
                      subTreeChanged: `NoChange,
                      oldInstance: <Text id=2 title="y" />,
                      newInstance: <Text id=2 title="y" />
                    }),
                    UpdateInstance({
                      stateChanged: true,
                      subTreeChanged: `NoChange,
                      oldInstance: <Text id=1 title="x" />,
                      newInstance: <Text id=1 title="x" />
                    })
                  ]
                )
            })
          ),
          actual
        );
        let updatedReactElement =
          Components.(
            listToElement([
              <Text key=key2 title="y" />,
              <Text key=key1 title="x" />
            ])
          );
        let actual2 =
          RenderedElement.update(
            ~previousReactElement,
            ~renderedElement=fst(actual),
            updatedReactElement
          );
        assertUpdate(
          ~label=
            "Updates the state because the state is updated with a new instance",
          (
            [<Text id=2 title="y" />, <Text id=1 title="x" />],
            Some({
              subtreeChange: `Reordered,
              updateLog:
                ref(
                  TestRenderer.[
                    UpdateInstance({
                      stateChanged: true,
                      subTreeChanged: `NoChange,
                      oldInstance: <Text id=1 title="x" />,
                      newInstance: <Text id=1 title="x" />
                    }),
                    UpdateInstance({
                      stateChanged: true,
                      subTreeChanged: `NoChange,
                      oldInstance: <Text id=2 title="y" />,
                      newInstance: <Text id=2 title="y" />
                    })
                  ]
                )
            })
          ),
          actual2
        );
      }
    ),
    (
      "Prepend Element",
      `Quick,
      () => {
        open ReasonReact;
        GlobalState.reset();
        GlobalState.useTailHack := true;
        let key1 = Key.create();
        let commonElement = [<Components.Text key=key1 title="x" />];
        let previousReactElement = listToElement(commonElement);
        let rendered0 = RenderedElement.render(previousReactElement);
        Assert.assertElement(
          [
            TestComponents.Text.createElement(
              ~id=1,
              ~title="x",
              ~children=(),
              ()
            )
          ],
          rendered0
        );
        let key2 = Key.create();
        let updated =
          RenderedElement.update(
            ~previousReactElement,
            ~renderedElement=rendered0,
            listToElement([
              <Components.Text key=key2 title="y" />,
              ...commonElement
            ])
          );
        assertUpdate(
          (
            [
              <TestComponents.Text id=2 title="y" />,
              <TestComponents.Text id=1 title="x" />
            ],
            Some({
              subtreeChange:
                `PrependElement([<TestComponents.Text id=2 title="y" />]),
              updateLog: ref([])
            })
          ),
          updated
        );
      }
    )
  ];

Alcotest.run(~argv=[|"--verbose --color"|], "Tests", [("BoxWrapper", suite)]);
