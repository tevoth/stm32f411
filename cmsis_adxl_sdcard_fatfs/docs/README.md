# Documentation Guide

This directory contains project-specific documentation for `cmsis_adxl_sdcard_fatfs`.

## Files

- `cmsis_adxl_sdcard_fatfs_chapter.tex`: follow-on chapter source
- `cmsis_adxl_sdcard_fatfs_chapter.pdf`: compiled output

## Prerequisite Reading

Read the raw-sector chapter first:

- `../cmsis_adxl_sdcard/docs/cmsis_adxl_sdcard_chapter.pdf` (repository-relative path)

This chapter intentionally focuses on what changes when adding FatFs.

## Build the PDF

Run from this directory:

```bash
latexmk -pdf -interaction=nonstopmode -halt-on-error cmsis_adxl_sdcard_fatfs_chapter.tex
```

`latexmk` automatically performs the required number of passes.
