TheoSys test system
===================
The test system is a test framework developed for **TPanel**. It can be activated during compile time by defining the preprocessor variable `TESTMODE` and assigning it the value `1`. Alternativley this variable can be defined in the file `testmode.h` too. The whole code must be (re)compiled with this preprocessor variable active. Then the resulting program has a new command line parameter:

* `-t <path>`

Where `<path>` is the path containing the test files. This can either be an absolute path or a relative. If this parameter is defined, the parameter `-c <config>` is ignored and must not be defined. Instead the directory with the test files contains a default configuration file called `testconfig.cfg`. It defines an empty surface what means that the built in default surface is created and loaded. All tests must be based on this surface. In case you want to use your own surface, change the path in the configuration file accordingly.

The test directory contains one or more files with the extension `.tst`. On startup **TPanel** reads all this files and executes the test cases in this files. Although it is possible to have only one file with all test cases inside, it is recomended to create a separate file for each test case. Name the files so that the name describes the test case.

Test case file
==============
A test case file is a simple text file. You can use any text editor you like to create or change them. A line starting with a hash (`#`) is a remark and will be ignored. All lines not containing a valid command are ignored. The simple interpreter does not stop if a invalid line is detected but writes a warning into the logfile.

All valid lines start with a keyword followed by an equal sign (`=`) and the content. There must be no space at the beginning of the line and before and after the equal sign. The following points describe the possible commands and their meaning.

Command: `command`
------------------
Defines the command to execute. This is the same syntax as described in the manual. If the command should trigger a particular button, the channel number must match the real channel number on the page or popup where the button is located.

**Example**:
`command=^BAT-3,1,-Local`

Appends to the text of the button with channel number 3 and the *off* state the text `-Local`.

Command: `port`
---------------
Defines the port number to use. By default port number 1 will be used. with this command a different port number can be used.

**Example**:
`port=15`

When the command is executed, the port number `15` will be used.

Command: `compare`
------------------
Defines whether the result should be verified or not. This can be set to `true` or `false`. Default is `false`!

If this is set to `true` the result is verfied against the one defined by the command `result`. The command succeeds only, if the result matches.

**Example**:
`compare=true`

Enabled the comparison against a defined result.

Command: `reverse`
------------------
Inverts the evaluation of the result. On success the result is treated as failure and on failure it is treated as success. This can be used to mark an expected failure as success.

**Excample**:
`reverse=true`

This means that the result is success if it failed in reality.

Command: `result`
-----------------
Defines a result. If the comparison was set to `true` (look at command `compare`) then the result of the last command is compared against the defined result. The command then succeeds only if the result matches this definition.

**Example**:
`result=Setup ...-Local`

For example, if the text of a button was set to `Setup ...-Local` then it matched the expected result.

Command: `wait`
---------------
This command let define a number of milliseconds the test machine should wait. Because the test machine is running in it's own thread, it does not block the rest of the program.

**Example**:
`wait=1000`

Wait 1000 milliseconds (1 second).

Command: `nowait`
-----------------
This command let define if the test machine should wait until a command finishes (default) or not. This can be set to `true` or to `false` (default). If this is set to `true`, then the test machine just executes the command (look at `command`) and don't wait. It also don't care about the result and goes immediately to the next command.

This can be usefull if a number of commands should be executed just like if they were send by a *NetLinx*.

**Example**:
`nowait=true`

Skips verification of result and jumps immediately to the next command.

Command: `exec`
---------------
This is the only command without any content. It executes the previously defined command, waits until it finished and takes the result from it. If comparison was enabled (look at command `compare`) it is compared against the expected result (look at command `result`) and then it reports whether the commond succeeded or not.

This command must be last one in a number of commands. Here is a complete example of a test case:

```
command=^BAT-3,1,-Local
result=Setup ...-Local
compare=true
exec
```

The command append the text `-Local` at the end of the text of a button. Then it compares the result against `Setup ...-Local`. It does this because the comparison was enabled (`cpmpare=true`).

It is possible to place as many command blocks as you like into one file. The all should look like the example above. The order of the commands doesn't matter as long as the command `exec` is the last one in a block.
