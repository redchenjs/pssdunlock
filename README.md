PSSD Unlock
===========

Unlock Samsung Portable SSD T3 / T5 under Linux.

## Dependencies

```
libusb-1.0
```

## Build

```
gcc pssdunlock.c -lusb-1.0 -o pssdunlock
```

## Unlock T3

```
./pssdunlock t3 <password>
```

## Unlock T5

```
./pssdunlock t5 <password>
```
