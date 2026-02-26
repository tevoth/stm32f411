# Per-Project Documentation Convention

Use this convention for every `cmsis_*` project in this repository.

## Directory Layout

Each project should own its documentation:

```
cmsis_<project_name>/
  docs/
    README.md
    <project>_chapter.tex
    <project>_chapter_body.tex
    <project>_chapter.pdf
```

## Required Files

- `docs/README.md`
  - purpose of the project docs
  - how to build PDF from LaTeX
  - expected filenames for chapter docs
- `<project>_chapter.tex`
  - standalone wrapper document (title page + TOC + body include)
- `<project>_chapter_body.tex`
  - includeable chapter content used by both standalone and repo-level master book
- `<project>_chapter.pdf`
  - compiled output for quick reading/sharing

## Build Command

From the project docs directory:

```bash
latexmk -pdf -interaction=nonstopmode -halt-on-error <project>_chapter.tex
```

`latexmk` automatically runs the required number of passes.

## Git Policy

- Commit `.tex` and `.pdf`.
- Do not commit LaTeX intermediate files (`.aux`, `.log`, `.out`, `.toc`, `.fls`, `.fdb_latexmk`, `.synctex.gz`).
- Keep docs scoped to the project directory, not in repository-level shared folders.
- Ensure chapter body files are compilable via repo-level master document for cross-chapter references.
