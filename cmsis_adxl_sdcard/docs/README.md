# Documentation Guide

This directory contains project-specific documentation for `cmsis_adxl_sdcard`.

## Files

- `cmsis_adxl_sdcard_chapter.tex`: chapter-style source document
- `cmsis_adxl_sdcard_chapter.pdf`: compiled output

## Build the PDF

Run from this directory:

```bash
latexmk -pdf -interaction=nonstopmode -halt-on-error cmsis_adxl_sdcard_chapter.tex
```

`latexmk` automatically performs the required number of passes.

## Notes

- This project chapter documents the raw-sector SPI2 microSD logger implementation.
- LaTeX intermediate files are intentionally ignored by git.
