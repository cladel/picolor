Picolor 🎨
=========

![Language](https://img.shields.io/badge/Language-c-blue)

Light-weight color picker for Linux X11 desktops. 

## Installation 🚧

Simply build it using `gcc main.c -o picolor`.

## How to use 🔍

Simply invoke the `picolor` command to start selecting colors.

``` shell
$ picolor
----------- Picolor-v0.1.0 -----------

Picker ON - Keyboard is now captured.
Press ESC to close the picker.
Press TAB to capture a color.
```

⚠ Keyboard input will be captured until `ESC` is pressed or 1 minute has passed, but you can still navigate with your mouse.

When pressing `TAB`, the current pixel color (hex and rgb value) will be printed to the standard output.
