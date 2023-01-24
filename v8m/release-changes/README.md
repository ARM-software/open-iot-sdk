# Release Changes Directory

This directory comprises information about all the changes made to the project since the last release.
A file should be added to this directory for each merge request. On release of the package, all of the
individual release change files, within this directory, are concatenated into a section of
the changelog in the root directory. All the release change files in this directory are
then automatically removed from the working tree and the index with `git rm`. However, they will still need to be committed before the full release.

## Changelog guideline

- Changelogs are for humans, not machines
- Communicate the impact of changes, use multiple paragraphs if needed
- Add a prefix that identifies the subsystem being changed, for example `mdh-arm:`

Examples:

```
docs: Add changelog guidelines
trusted-firmware-m: Use srec_cat for combining images
ci: Avoid refetching build dependencies in the same job
examples: Remove workaround for RTX crash
```

For more details, please follow our [`towncrier` guidelines](https://gitlab.arm.com/iot/open-iot-sdk/tools/developer-tools/-/blob/main/templates/towncrier/README.md).
