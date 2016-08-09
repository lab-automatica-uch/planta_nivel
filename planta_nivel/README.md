Planta para control de nivel
============================

Para compilar en Windows usar:

```
mex -lWSock32 -Iinclude src/SPlantaNivel.cpp src/opto22snap.cpp
```

En Linux

```
mex -D_LINUX -Iinclude src/SPlantaNivel.cpp src/opto22snap.cpp
```
