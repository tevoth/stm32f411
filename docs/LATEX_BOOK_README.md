# LaTeX Book Build Guide

This repository now supports two documentation build modes:

1. Per-project standalone chapter PDFs (`cmsis_*/docs/*_chapter.tex`)
2. A repo-level combined book (`stm32_projects_book.tex`)

## Build the Combined Book

From repository root:

```bash
latexmk -pdf -interaction=nonstopmode -halt-on-error stm32_projects_book.tex
```

Output:

- `stm32_projects_book.pdf`

Optional cleanup:

```bash
latexmk -c stm32_projects_book.tex
```

## Why a Combined Book

- allows cross-chapter references via `\label` and `\ref`
- keeps each chapter owned by its project directory
- avoids duplicated content across standalone and aggregate docs

## Current Included Chapters

- `cmsis_adxl_sdcard/docs/cmsis_adxl_sdcard_chapter_body.tex`
- `cmsis_adxl_sdcard_fatfs/docs/cmsis_adxl_sdcard_fatfs_chapter_body.tex`
