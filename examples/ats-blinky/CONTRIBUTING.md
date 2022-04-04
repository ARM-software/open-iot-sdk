## Contributing

## License details

Please use an [SPDX license identifier](http://spdx.org/licenses/) in every source file following the [recommendations](https://spdx.dev/spdx-specification-21-web-version/#h.twlc0ztnng3b) to make it easier for users to understand and review licenses.

The full original [Apache 2.0 license text](http://www.apache.org/licenses/LICENSE-2.0) is in [LICENSE-apache-2.0.txt](LICENSE-apache-2.0.txt). Each source file should start with your copyright line followed by the SPDX identifier as shown here:

```
/* Copyright (c) 2022, Arm  Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
```

### License check

We use scancode-toolkit to enforce the license in binaries and libraries.

To verify file licenses (using CHANGED-FILES as an example):
```
scancode -l --json-pp scancode_report.json CHANGED-FILES
jsonschema -i scancode_report.json .gitlab/ci/license/license.schema
```

## Code Style checks

Source code in this repository is formatted with [Clang-Format Styling](https://clang.llvm.org/docs/ClangFormatStyleOptions.html). Style format config we use is [available here](./.clang-format). We use `clang-format` version 13.

Every contribution to this repository is checked for Clang-Format styling in our Continuous Integration (CI). It is a very good practice to run clang-format style checks locally in your development environment before creating a Merge Request to this repository to save time.

Depending on the development environment, installation instructions for clang-format would vary. Please refer to [official documentation](https://llvm.org/) for installation instructions.

Once, you have `clang-format` installed, you can run style code by executing as given below

Please note that we use clang-format-13, make sure that you have installed the same version.

```
# To format a single file
clang-format -i <path_to_file>
# To batch format files
git ls-files '*.h' '*.c' '*.cpp' | xargs clang-format -i
```

💡 If you have Docker installed, you can run the clang-format styling as given below. This makes use of same docker image used in our CI.

```
cd path/to/source/code/checkout/from/this/repository
docker run --rm -v $PWD:/app -w /app mcudriverhal/ci-build-env:clang-format-13 /bin/bash -c "git ls-files '*.h' '*.c' '*.cpp'| xargs clang-format -i"
```

## Developer’s Certificate of Origin

Each commit must have at least one `Signed-off-by:` line from the committer to certify that the contribution is made under the terms of the [Developer Certificate of Origin](./DCO.txt).

If you set your `user.name` and `user.email` as part of your git configuration, you can add a signoff to your commit automatically with `git commit -s`. You have to use your real name (i.e., pseudonyms or anonymous contributions cannot be made). This is because the DCO is a legally binding document.

## Pull request guidelines

- Git commit messages should be concise and must contain a proper description of the commit content. We recommend following Chris Beams 7 rules https://chris.beams.io/posts/git-commit/#seven-rules.
- Add a prefix to your commit message to highlight the area of relevance. Write prefixes capitalized (only the first letter in uppercase, the rest in lowercase), followed by a colon, followed by a space, and then finally the commit title.
- Keep a linear git history: no merge commits are allowed, always rebase your branch.
- Preserve the history until the review is completed. Use `git commit --fixup` to add fixes. Once review is completed, run `git rebase --autosquash` to clean the history.
- Every new addition to an already opened pull request should be commented to inform the reviewers on what has changed.

## Pull request process

PRs should go through the following steps:

To be defined per repository

## Codeowners

Technical ownership of the codebase falls on the codeowners and all contributions are ultimately merged by the them. The members are listed in the [CODEOWNERS](CODEOWNERS) file.

## Maintainers

Maintainers provide guidence and direction to contributors.

Martin Kojtal
    :email: `Martin Kojtal <Martin.Kojtal@arm.com>`
    :gitlab: `https://gitlab.arm.com/Martin.Kojtal`

Anna Bridge
    :email: `Anna Bridge <Anna.Bridge@arm.com>`
    :gitlab: `https://gitlab.arm.com/Anna.Bridge`
