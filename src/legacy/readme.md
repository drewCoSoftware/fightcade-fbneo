# Legacy Stuff

## CustomBuilds
This folder contains files that used to have custom build steps in the visual studio compiler.
Since they were only used to generate static header files, their output was copied to **src/generated**.

Refer to commit: __f24f55f9ca6481294d645350f0ea1262e5fd1baa__
For specific syntax of the custom build steps.

**NOTE:** The file <u>build_details.cpp</u> is a program that uses preprocessor directives to create a header file that user other preprocessor directives depending on the platform.  This is all used to
create a header file for the sake of the about box to describe when the binary was built, what platform, etc.  This data is static now, but IMO isn't particularly useful to the end users anyway.  We can find
a better way to generate this information in the future if anyone cares.

## Scripts
This folder contains all of the PERL scripts that were used to generate static header files.  An example of how those scripts were run are contained the file **games.bat**.  Note that this batch file uses paths / arguments that will probably not work.  This file is included only as an example of how the legacy scripts may have been run in the past.




