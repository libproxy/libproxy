Title: Build steps - How to compile libproxy
Slug: building

# Build steps - How to compile libproxy

## Fedora

### Dependencies

```
sudo dnf install glib2-devel duktape-devel meson gcovr gi-docgen libcurl-devel vala gsettings-desktop-schemas-devel gobject-introspection-devel
```

### Build Setup

```
meson setup build
```

### Compilation

```
ninja -C build
```

### Installation

```
ninja -C build install
```

## OS X

### Dependencies

```
pip install meson ninja
brew install icu4c gobject-introspection duktape gcovr gi-docgen curl vala
```

### Build Setup

```
meson setup build
```

### Compilation

```
ninja -C build
```

### Installation

```
ninja -C build install
```

## Windows (MSYS2)

### Dependencies

```
pacman -S base-devel git mingw-w64-x86_64-toolchain mingw-w64-x86_64-ccache mingw-w64-x86_64-pkg-config mingw-w64-x86_64-gobject-introspection mingw-w64-x86_64-python-gobject mingw-w64-x86_64-meson mingw-w64-x86_64-glib mingw-w64-x86_64-duktape mingw-w64-x86_64-gi-docgen mingw-w64-x86_64-curl mingw-w64-x86_64-vala
```

### Build Setup

```
meson setup build
```

### Compilation

```
ninja -C build
```

### Installation

```
ninja -C build install
```

