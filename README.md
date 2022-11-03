# ISO2UL
 command line tool to convert ISO9660 images into segmented 'USBExtreme' format file for usage on OpenPS2Loader
 original source code provided by @HowlingWolfHWC
 
 
## usage
```
ISO2UL.exe IMAGEFILE ROOTPATH "TITLE" [CD/DVD]
```
where:
- `IMAGEFILE` is a path to the game file
- `ROOTPATH` is the location of the OPL Folders structure (eg: usb device root, or a sub-folder if you use the prefix path features)
- `TITLE` game title to be shown on OPL, max `32` chars
- `CD/DVD` media type

Example:

```
ISO2UL.exe "I:\games\foo.ISO" "D:\GAMES" "my 32 chars title" CD
```


