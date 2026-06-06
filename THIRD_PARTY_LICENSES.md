# Third-party licenses

`makexx` itself is MIT-licensed (see [LICENSE](LICENSE)). The interactive
dependency-graph viewer (`make graph` → `makefile_graph.html`) bundles the
following open-source JavaScript libraries so the generated graph is a single
self-contained, offline file. They are vendored under `assets/vendor/`, embedded
into the `makexx` binary at build time, and inlined into every generated
`makefile_graph.html`. All are distributed under the MIT License — full credit to
their authors and communities.

| Library | Used for | Project |
|---|---|---|
| **Cytoscape.js** (v3.30.2) | graph rendering / interaction | <https://js.cytoscape.org> |
| **dagre** | layered DAG layout | <https://github.com/dagrejs/dagre> |
| **cytoscape-dagre** | dagre adapter for Cytoscape.js | <https://github.com/cytoscape/cytoscape.js-dagre> |
| **cytoscape-expand-collapse** | foldable group/compound nodes | <https://github.com/iVis-at-Bilkent/cytoscape.js-expand-collapse> |
| **cytoscape-svg** (v0.4.0) | vector (SVG) export | <https://github.com/kinimesi/cytoscape-svg> |

Verified copyright notices from the vendored files:

- Cytoscape.js — Copyright (c) 2016–2024, The Cytoscape Consortium.
- dagre — Copyright (c) 2012–2014 Chris Pettitt.

The remaining extensions (`cytoscape-dagre`, `cytoscape-expand-collapse`,
`cytoscape-svg`) are likewise MIT-licensed by their respective authors; see each
project's repository above for the full license text and copyright.

## The MIT License

All of the above are made available under the MIT License:

```
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

The static PDF graph (`make makefile_graph.pdf`) shells out to
[Graphviz](https://graphviz.org/) if installed; Graphviz is **not** bundled — it
remains a separate, optional system dependency under its own (EPL) license.
