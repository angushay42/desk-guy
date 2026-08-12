# desk-guy

## PlatformIO Integration
### Bring Up
- As part of my first attempt at using ESP-IDF and PlatformIO, I have created some bring-up tests for various subsystems.
- Source files for these live in /src/bring-up and act as their own app, with one definition of `app_main`
- `platformio.ini` configures an environment for each bring-up test, which essentially sets the `$PIOENV` variable to that test
    - `generate_bringup.py` is generated code from Claude. I am unsure exactly how the first statement works: `Import("env")` but it is apparently part of the PlatformIO build system
- There are also multiple `sdkconfig.<bring-up-test>` files, which can mess with Intellisense; **however**, if you select the specific environment on the bottom tab of VSCode then it should figure itself out.

> [!NOTE] 
> if the sdkconfig files don't exist, the project must be built first.

 
