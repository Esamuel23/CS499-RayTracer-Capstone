# CS 499 Code Review

**Elijah Samuel** · CS 499 Computer Science Capstone · Southern New Hampshire University

This is my code review of the two artifacts I am enhancing for the capstone. It is the complete written record of the review; the accompanying captioned screencast walks through the same code and findings on screen.

---

## Purpose

I reviewed both artifacts in their original state, before any enhancement, so that each enhancement has a fixed baseline to be measured against. Every defect recorded here is intentionally left in place. Fixing them is the work of the enhancement modules, and each fix will point back to the finding that motivated it.

## Scope and provenance

**Artifact One — CS 330 3D scene renderer.** C++ against OpenGL, GLFW, GLEW, and GLM, built for Visual Studio 2022 (toolset v143). The snapshot in `original/` is the module 3 export. I reviewed `MainCode.cpp`, `SceneManager.cpp`, `SceneManager.h`, `ViewManager.cpp`, and `ShaderManager.cpp` — 1,029 lines.

`ShapeMeshes.cpp` and `ShapeMeshes.h`, roughly 2,400 lines of mesh generation, were provided with the course and I did not modify them. They are a dependency of the artifact, not my authored work, and I have not reviewed them as mine.

**Artifact Two — CS 340 animal shelter dashboard.** Python against MongoDB, built for classic Jupyter Notebook on SNHU's Apporto virtual desktop. I reviewed `animal_shelter.py` and cell 0 of `dashboard.ipynb` — 276 lines. Notebook line numbers throughout count from the first line of cell 0 of the committed version.

## Method

I worked both artifacts against the CS 499 code review checklist: structure, documentation, variables, arithmetic operations, loops and branches, and defensive programming.

I followed SmartBear's peer review guidance for the inspection itself: under 400 lines per sitting, under 500 lines per hour, and no session longer than an hour. The 1,305 lines here were inspected file by file across several sittings within those limits. The screencast is not the inspection — it is the report-out of findings already made, which is why it covers far more code per minute than the inspection rate would allow.

SmartBear's case for checklists is that omissions are the hardest defects to find, because it is difficult to review something that is not there. That held here. The most consequential finding in Artifact One is a missing `else` branch, which is exactly the kind of defect a checklist item catches and a read-through misses.

## Format

The screencast is captioned and silent. My recording environment could not produce audio of a quality I was willing to attach to a portfolio piece, so I chose on-screen captions over narration. The captions carry the findings; this document carries the full detail, including files the screencast only passes over.

---

# Artifact One — CS 330 Scene Renderer

## MainCode.cpp

The entry point and render loop.

- **L18-32 — no ownership model.** Every major object is a raw global pointer in an anonymous namespace: the window, the scene manager, the shader manager, and the view manager. Lifetime is managed by hand.
- **L21 — comment inconsistent with code.** The comment calls `WINDOW_TITLE` a macro. It is a `const char* const`.
- **L46-77 — unenforced ordering.** Startup runs GLFW, then the shader manager, view manager, window, GLEW, shaders, and scene. The order is load-bearing and nothing enforces it.
- **L61 — unchecked return.** `CreateDisplayWindow` can return NULL. The result is used at L81 without a check.
- **L64-67 — leak on the failure path.** By the time GLEW is checked, the shader manager and view manager are already on the heap. This early return deletes neither.
- **L70 — discarded return value.** `LoadShaders` returns zero on failure. The value is thrown away, and L73 calls `use()` on a program that may not exist.
- **L84, L87 — loop-invariant statements inside the loop.** `glEnable(GL_DEPTH_TEST)` and `glClearColor` are re-issued every frame for the life of the program. Neither value changes. Both belong above L81.
- **L105-119 — repeated block.** The same check-delete-null pattern is written out three times.
- **L122 — devices not left in a clean state.** `exit(EXIT_SUCCESS)` is used instead of `return`. `glfwTerminate()` is never called, so GLFW and the OS window are torn down by process exit rather than by the library.
- **L130-150 — the signature does not match the body.** `InitializeGLFW` returns `bool`, and L149 returns `true` unconditionally. L134 calls `glfwInit()` and ignores its result. By contrast, `InitializeGLEW` at L157 does test its result at L165.

## SceneManager.cpp

Where most of my authored code lives, and where the most serious defects are.

- **L1-8 — comment inconsistent with code.** The file header names `shadermanager.cpp`. This is `SceneManager.cpp`.
- **L20-27 — symbolics used correctly.** Five shader uniform names are named constants. This is the pattern the rest of the file should follow.
- **L34-38 — uninitialized member.** The constructor sets `m_pShaderManager` and `m_basicMeshes` but never initializes `m_loadedTextures`. That member is the index written at L113 and the loop bound at L134 and L150.
- **L45-50 — rule of three unsatisfied.** Raw `new` in the constructor and raw `delete` in the destructor, with no copy constructor, copy assignment, or move operations. Copying a `SceneManager` double-frees.
- **L94-103 — leak on an error path.** The if / else-if / else is complete, which is correct. But the `else` at L102 returns without calling `stbi_image_free`, so the image buffer allocated at L71 leaks on exactly the path that reports failure.
- **L113 — index not tested against bounds.** `m_textureIDs[m_loadedTextures]` has no bounds check against the 16-element array declared at `SceneManager.h` L56 — and the index was never initialized.
- **L148-154 — the most serious defect in the file.** `DestroyGLTextures` calls `glGenTextures` at L152 where it should call `glDeleteTextures`. It generates new texture names into the slots it claims to free. Every loaded texture leaks, behind a function whose name says the opposite.
- **L162-206 — repeated code.** `FindTextureID` and `FindTextureSlot` are the same eighteen-line linear scan. They differ only in whether L172 or L198 returns the ID or the index.
- **L214-241 — output variable not assigned.** `FindMaterial` returns `false` at L216 when the list is empty, but returns `true` unconditionally at L240 whether or not a match was found. A caller asking for a missing material gets `true` and an untouched output struct.
- **L223 — type inconsistency.** `int index` is compared against `m_objectMaterials.size()`, which is `size_t`.
- **L275 and throughout — coding standard.** `NULL` is used rather than `nullptr`.
- **L355-362 — the defect propagates.** `SetShaderMaterial` trusts the `FindMaterial` return value and reads the output struct's indeterminate values into the shader.
- **L358-362 — magic string constants.** The five material uniform names are raw literals, the same category of value that L20-27 handles correctly. A typo here fails silently, because a bad uniform name is not an error in OpenGL.
- **L367-372 — leftover scaffolding.** The "STUDENTS CAN MODIFY" course template banner is still in the file.
- **L382-390 — unreachable procedures.** `PrepareScene` loads two meshes and `RenderScene` uses flat colors only. `CreateGLTexture`, `BindGLTextures`, `DestroyGLTextures`, `FindTextureID`, `SetShaderTexture`, `SetTextureUVScale`, and `SetShaderMaterial` have no call sites. I confirmed this by search: each appears only as its own definition and its own doc banner. The entire texture and material subsystem is dead code in this snapshot.
- **L416-446 — magic numbers.** The scene geometry — the ground plane scale, the desk dimensions, the leg positions, and the color triples — is written entirely as unnamed literals. The relationship that actually has to hold, between the leg positions and the desk dimensions, is not expressed anywhere.
- **L434, L441 — the literal 4, twice.** The leg array is sized 4 and the loop is bounded by 4. Change one and they silently disagree. The loop itself is correct: the index is initialized immediately before, termination is obvious, and the index is neither mutated in the body nor used after exit.

## SceneManager.h

- **L32-36 — type inconsistency.** `TEXTURE_INFO::ID` is `uint32_t`, while `FindTextureID` returns `int` and signals "not found" with -1, a value that cannot exist in the field's own type.
- **L56 — inefficient storage and an unnamed bound.** `TEXTURE_INFO m_textureIDs[16]` allocates sixteen structs, each holding a `std::string`, for the life of the object whether one texture is loaded or none. The 16 has no name, and it is the bound L113 of the .cpp fails to check.
- **L58 — replaceable by a library container.** `m_objectMaterials` is a `std::vector` scanned linearly by tag. A `std::unordered_map` keyed on tag replaces the scan and the duplicated finder pattern at once.

## ViewManager.cpp

- **L2 — comment inconsistent with code.** The header comment names `viewmanager.h` in a `.cpp` file. The same copied-banner defect as `SceneManager.cpp`.
- **L28, L68-78 — ownership confusion.** `g_pCamera` is a file-scope global but is allocated in the constructor and deleted in the destructor of an instance. Two `ViewManager` objects would double-free it.
- **L31-33, L41 — unused variables.** `gLastX` and `gLastY` are initialized and never read. `gFirstMouse` is set and never tested. `bOrthographicProjection` is declared and never referenced.
- **L36, L157 — write-only variable.** `gDeltaTime` is assigned every frame at L157 and never read.
- **L60 — type consistency.** `g_pCamera->Zoom = 80` assigns an integer literal to a float member.
- **L104 — dead code.** A commented-out `glfwSetInputMode` call is left in place.
- **L107, L124-126 — leftover stub.** L107 registers `Mouse_Position_Callback` with GLFW as a live callback. Its body at L124-126 is empty. Mouse input is wired to nothing.
- **L134-141 — incomplete against the design.** Only the Escape key is handled. A camera with position and orientation exists, but there is no movement input, so the scene is a fixed viewpoint rather than an explorable one.
- **L156 — implicit narrowing.** `glfwGetTime()` returns `double` and is assigned to `float currentFrame`.
- **L165 against L171 — inconsistent defensive posture.** L165 dereferences `g_pCamera` with no null check. L171, six lines later, null-checks `m_pShaderManager`. Two pointers in the same function held to two different standards.
- **L168 — divisor and magic numbers.** `WINDOW_WIDTH / WINDOW_HEIGHT` is safe, because both are nonzero compile-time constants — this is the one place the zero-divisor check passes. But the aspect ratio is fixed to the startup resolution rather than read from the framebuffer, so it is wrong once the window is resized. The near and far planes, 0.1 and 100.0, are unnamed.

## ShaderManager.cpp

- **L8 — coding standard.** `using namespace std;` at global scope in a translation unit that includes headers.
- **L26-27 — resources created before they can be validated.** Both shader objects are created before either file is read.
- **L32-41 — the vertex shader read handles failure.** The `else` at L37 reports the failure and returns zero.
- **L39 — leftover debug routine.** `getchar()` inside a windowed OpenGL application blocks on standard input that no user is watching.
- **L40 — leak on the failure path.** This early return does not call `glDeleteShader` on either object created at L26-27.
- **L45-51 — the missing else. The most consequential defect in Artifact One.** The fragment shader read, eleven lines after the vertex read, has no `else` at all. If the fragment file is missing, `FragmentShaderCode` stays empty, execution continues, an empty shader is compiled at L76-78, and L89 prints "success". A missing file produces a success message.
- **L64-72 — a check that checks nothing.** L64 reads the compile status into `Result`, and `Result` is never tested. L66 branches only on log length, and L72 prints "success" unconditionally. The same pattern repeats at L81-89 and L100-108. The comments say "Check Vertex Shader" and "Check the program"; no check occurs.
- **L66, L83 against L102 — inconsistent threshold.** The same condition is written `InfoLogLength > 0` twice and `InfoLogLength > 1` once, with no explanation.
- **L94 — two channels for one result.** `m_programID` is set as a side effect and the same value is returned. The caller at `MainCode.cpp` L70 uses neither.
- **Style.** This file uses `printf` and K&R braces; the rest of the codebase uses `std::cout` and Allman braces.

## Arithmetic operations

Artifact One passes the arithmetic section of the checklist by omission rather than by design. There are no floating-point equality comparisons, no accumulation loops that raise rounding concerns, and the one division, at `ViewManager.cpp` L168, has constant nonzero operands. That is a consequence of the code doing very little math — nearly every value is a literal handed to GLM.

That changes completely under the enhancement. A ray tracer is arithmetic. Intersection tests compare a ray parameter against a minimum, and comparing it against exactly zero is the classic self-intersection bug, so I will carry an explicit epsilon. The slab test for box intersection divides by each component of the ray direction, and a zero component — a ray parallel to the slab — has to be handled rather than allowed to produce infinity. Color accumulated across many samples is addition across widely differing magnitudes.

## What the enhancement changes

Raw owning pointers become smart pointers, which resolves the rule-of-three problem and the leaks. The fixed sixteen-slot array becomes a container that knows its own size, which removes the unchecked index. The duplicated finders collapse into a single lookup. The magic numbers in `RenderScene` become named scene constants. The dead texture and material subsystem is either used or removed. The error handling in `ShaderManager` is replaced outright, because a CPU ray tracer has no shader files to load and no OpenGL context to leave in a bad state.

---

# Artifact Two — CS 340 Animal Shelter Dashboard

## animal_shelter.py

The CRUD layer over MongoDB.

- **L5 — Python 2 idiom.** `class AnimalShelter(object)` uses an explicit base class that Python 3 does not need.
- **L6 — inadequate documentation.** The class docstring is the only docstring in the file. None of the five methods has one.
- **L9-17 — comments inconsistent with the code's current state.** A seven-line comment block, with three misspellings and three blank comment lines, documents an environment the code no longer runs in.
- **L18-21 — hardcoded configuration.** Host, port, database, and collection are locals inside `__init__`, not module constants, configuration, or parameters.
- **L24 — three defects in one line.** The credentials are percent-formatted directly into the URI, so a password containing `@` or `:` corrupts it; `urllib.parse.quote_plus` exists for this. There is no `try`/`except` around client construction. And there is no `serverSelectionTimeoutMS`, so a wrong host hangs for the thirty-second default with no error trap.
- **L25 — redundant operation.** `self.client['%s' % (DB)]` formats a literal string into a literal string.
- **L26 — unused variable, and a contradiction.** `self.collection` is built from the `COL` constant and never read. Every method — L31, L37, L42, L49 — hardcodes `self.database.animals` instead, which makes the `COL` constant at L21 decorative.
- **L30 against L41 — inconsistent input validation.** `create` tests `if data is not None`, which accepts an empty dict and inserts an empty document. `update` uses a truthiness test and rejects empties. Two methods in the same class, two different standards.
- **L34, L45, L52 — unspecific exceptions.** All three raise bare `Exception`, so a caller cannot distinguish a validation failure from a connection failure.
- **L35, L40 — formatting.** No blank line between method definitions (PEP 8). L39 is indented three spaces where every other body uses four.
- **L37 — storage efficiency.** `list(...)` materializes the entire result set in memory, with no projection and no limit.
- **Whole file — no teardown.** There is no `close` method and no context manager support, so the `MongoClient` connection is left open for the life of the process.

## dashboard.ipynb

The Dash application. Line numbers count from the first line of cell 0.

- **L22 — the notebook does not run as committed.** `from AnimalShelter import AnimalShelter` imports a module that does not exist. The file is `animal_shelter.py`. The FIX ME at L20-21 says to change this, and it was not changed.
- **L20, L27, L55, L59, L60, L98, L99, L130 — leftover scaffolding.** Eight FIX ME markers remain. Several sit directly above code that was in fact written, which makes them actively misleading.
- **L29-30 — imported data not validated.** Credentials are read from the environment rather than hardcoded, which is an improvement. But neither is checked. If a variable is unset, `os.environ.get` returns `None`, which is then formatted into the connection URI as the literal string "None".
- **L35 — comment inconsistent with code.** The comment says `read` accepts projection input. It does not.
- **L43 against L144-145 — inconsistent defensive posture.** L43 drops the `_id` column unguarded and raises `KeyError` on an empty collection. L144-145 performs the identical drop inside an existence check.
- **L45-47 — leftover test routines.** Commented-out debug prints.
- **L56-57 — file accessed without an existence check.** `open('my-image.png', 'rb')` has no existence check and no `try`, and the file is not in the repository, so this raises `FileNotFoundError` at import time. The handle is never closed, because `open` is called inline as an argument rather than in a `with` block.
- **L131-140 — most common case tested last.** The chain does have its `else` at L139. But the default, `None`, is the value the dashboard starts with and therefore the most common path, and it is tested last, after four string comparisons that fail.
- **L132-138 — incomplete against the design, and magic strings.** Each rescue type matches a single breed, written as an unnamed string literal. The Grazioso specification calls for several breeds per type, plus sex and an age range.
- **L142-146 — repeated block.** Read, build a frame, drop `_id`: the same work as L37-43, written twice and guarded differently each time.
- **L160 — defended correctly.** `breed` is explicitly checked before the chart is built.
- **L198-202 — index not tested against bounds.** `row` defaults to 0 or takes `index[0]`, and neither path checks it against the length of the frame. The explanatory comment at L202 sits after the block it describes.
- **L205 — comment inconsistent with code.** "Austin TX is at [30.75,-97.48]" sits above a map that centers on the selected row, not on Austin.
- **L208-215 — unvalidated data.** `location_lat`, `location_long`, `breed`, and `name` are each accessed with no existence check, in contrast to L160. The same `dff.loc[row, ...]` lookup is written four times in one statement.
- **L10 — unused import.** `State` is imported and never used.
- **L2 — deprecated dependency.** `jupyter_dash` has been superseded by Dash's own notebook support.
- **L223 — leftover debug setting.** `debug=True` is in the committed file, which exposes an interactive debugger in the browser.

## Environment and dependencies

The repository contains no `requirements.txt`, `environment.yml`, `Pipfile`, or `pyproject.toml`. Six third-party dependencies — `pymongo`, `dash`, `dash_leaflet`, `plotly`, `pandas`, and `jupyter_dash` — are imported and none is declared. `animal_shelter.py` L18 hardcodes an Apporto lab host reachable only inside the classroom environment.

So the artifact is tied to an environment that no longer exists, with no manifest describing how to rebuild it anywhere else. Together with the import at L22 and the missing image at L57, that is three independent reasons a reader cannot run it. Under the checklist, this is both a coding-standards failure and an incomplete implementation of the design: a design that cannot be stood up is not fully delivered.

## What the enhancement changes

A pinned dependency manifest. The connection supplied by environment against a managed MongoDB Atlas cluster rather than hardcoded to a lab host, with the credentials URL-encoded, validated, and bounded by a timeout. Schema validation on the collection and indexes chosen against the queries the dashboard actually runs. A least-privilege database user. The migration off `jupyter_dash`. And consistent validation across the CRUD methods.

---

# Checklist coverage

| Checklist item | Where it appears |
|---|---|
| Implements the design completely and correctly | ViewManager L134-141 · dashboard L22, L132-138 · no dependency manifest |
| Conforms to coding standards | MainCode, SceneManager `NULL` · ShaderManager L8 · animal_shelter L5, L35-40 |
| Consistent style and formatting | ShaderManager `printf` and K&R · animal_shelter L39 |
| Unneeded procedures or unreachable code | SceneManager texture and material subsystem · ViewManager L104 |
| Leftover stubs or test routines | ViewManager L124-126 · ShaderManager L39 · SceneManager L367-372 · dashboard FIX MEs, L45-47, L223 |
| Replaceable by library components | SceneManager.h L58 · animal_shelter L24 |
| Repeated code | MainCode L105-119 · SceneManager L162-206 · dashboard L142-146 |
| Storage efficiency | SceneManager.h L56 · animal_shelter L37 |
| Symbolics over magic numbers and strings | SceneManager L358-362, L416-446, L434/441 · SceneManager.h L56 · dashboard L132-138 |
| Modules too complex | `RenderScene` · `LoadShaders` |
| Adequate documentation | animal_shelter L6 · no docstrings on methods |
| Comments consistent with code | MainCode L21 · SceneManager L1-8 · ViewManager L2 · ShaderManager L64-72 · dashboard L35, L205 |
| Meaningful, consistent names | `g_` and `m_` prefixes applied inconsistently across files |
| Type consistency and casting | SceneManager L223 · SceneManager.h L32-36 · ViewManager L60, L156 |
| Redundant or unused variables | ViewManager L31-41, L36 · animal_shelter L26 · dashboard L10 |
| Floating-point equality | None present — passes by omission |
| Rounding errors, magnitude differences | None present — passes by omission |
| Divisors tested for zero | ViewManager L168 — constant nonzero operands, passes |
| Loops and branches complete and nested | SceneManager L434-446 correct · ShaderManager L45-51 missing `else` |
| Most common case tested first | dashboard L131-140 — tested last |
| All cases covered, including `else` | ShaderManager L45-51 · SceneManager L94-103 correct |
| Loop-invariant statements hoisted | MainCode L84, L87 |
| Indexes and pointers tested against bounds | SceneManager L113 · ViewManager L165 · dashboard L198-202 |
| Imported data and arguments validated | MainCode L61, L70 · animal_shelter L30 · dashboard L29-30, L208-215 |
| All output variables assigned | SceneManager L214-241 |
| Every allocation deallocated | SceneManager L102, L148-154 · MainCode L64-67 · ShaderManager L40 |
| Timeouts and error traps on external access | animal_shelter L24 |
| Files checked before access | ShaderManager L45-51 · dashboard L56-57 |
| Files and devices left in a correct state | MainCode L122 · animal_shelter, no teardown |

---

# Summary

Artifact One is sound at the class level and unsound at the error-handling level. It has a cleanup function that leaks instead of freeing, a finder that reports success on failure, an uninitialized index into a fixed array, and a shader loader that prints "success" when its input file is missing.

Artifact Two does not run as committed, because of an import that names the wrong module, an image file that is not in the repository, and an environment that cannot be rebuilt. Beneath that, its real weakness is inconsistency: the same operation defended in one place and not another, a collection constant contradicted by every method that should use it, and validation rules that differ between methods of the same class.

Both artifacts were written to work on the happy path and were never asked what happens otherwise. The enhancements are my direct answers to these specific defects.
