# fuse-clipboard
# Usage
## Prerequisites
Requires Qt6 and fuse3

To mount the filesystem at `mount-dir`, run
```bash
fuse-clipboard <mount-dir>
```

The directory you are mounting over should be empty.

## Directory structure
The general directory structure is as follows, with `/` being mount point

```
/
    clipboard/
        <mime-type>/
		file.<extension>
		file.<extension>
		...
	<mime-type>/
		file.<extension>
		...
	...
```

At the moment, the names of files is always 'file' followed the the mime subtype as stored in the clipboard

As an example, if you copied on image in Firefox, the file structure might look something like this
```
/
	clipboard/
		application/
			file.ico
			file.x-qt-image
		image/
			file.bmp
			file.ico
			file.icon
			file.jpeg
			file.png
			file.tiff
			file.vnd.microsoft.icon
			file.x-MS-bmp
			file.x-bmp
			file.x-ico
			file.x-icon
			file.x-win-bitmap
		text/
			file._moz_htmlcontext
			file._moz_htmlinfo
			file.html
			file.ico
```

## Unmounting
```bash
fusermount -u <mount-dir>
```

## Caevats
For certain mime subtypes under certain mime types, the file may be empty. This is due to the underlying library used to access the clipboard (at the moment, only Qt) not returning data.


# Building
Can build with Qt creator and CMake. On Arch Linux, install:
- qt-creator
- qt6-base
- cmake

In Qt creator, you may need to add qt6 manually.

In the menu bar, go to Edit -> Preferences. Find kits in the side scrolling bar. After selecting kits, select "Qt Versions". On the right side, click "Add..." and add `/usr/bin/qmake6` to add Qt6.

Then open the project's `CMakeLists.txt` file and build.
