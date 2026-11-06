# Кросс-сборка FPRecorder.exe из Linux (без Visual Studio)

Собрано 2026-11-01 в контейнере без компилятора и без root: Zig 0.17 + mingw-w64
заголовки. Итог: `build/FPRecorder.exe`, x64, подсистема Windows, 1.8 МБ,
UCRT-импорты (Windows 10/11).

## Что понадобилось

```bash
pip install --user ziglang        # бинарник: ~/.local/*/lib/python*/site-packages/ziglang/zig
```

Zig 0.17 не имеет `<stacktrace>` в своём libc++, а mingw-w64 не имеет двух
заголовков MSVC. Поэтому рядом лежит оверлей `scratch/build/ci` (создаётся
скриптом ниже) и голова `scratch/build/gui -> GUI` (Linux чувствителен к регистру,
а код включает `gui/gui.h`).

## Сборка

```bash
cd projects/fprecorder
Z=~/.local/*/lib/python3*/site-packages/ziglang/zig
export ZIG_LOCAL_CACHE_DIR=/tmp/superlisa/zig-local ZIG_GLOBAL_CACHE_DIR=/tmp/superlisa/zig-global

mkdir -p scratch/build/ci tmp
ln -sfn "$PWD/GUI" scratch/build/gui
printf '#pragma once\n#include <shlobj.h>\n' > scratch/build/ci/shlobj_core.h
# audioclientactivationparams.h — определения AUDIOCLIENT_ACTIVATION_PARAMS и
# VIRTUAL_AUDIO_DEVICE_PROCESS_LOOPBACK, которых нет в mingw-w64.
# stacktrace — минимальная замена std::stacktrace на CaptureStackBackTrace.

F="-target x86_64-windows-gnu -std=c++23 -O2 -DNDEBUG -DWIN32 -DUNICODE -D_UNICODE -Iscratch/build/ci -I. -Iscratch/build -IGUI"
$Z c++ $F -c GUI/gui.cpp   -o /tmp/superlisa/obj/gui.o
$Z c++ $F -c Provider.cpp  -o /tmp/superlisa/obj/Provider.o
$Z cc  -target x86_64-windows-gnu -O2 -DNDEBUG -c stb_vorbis.c -o /tmp/superlisa/obj/stb_vorbis.o
$Z c++ $F -c main.cpp      -o /tmp/superlisa/obj/main.o

$Z c++ -target x86_64-windows-gnu -O2 -municode /tmp/superlisa/obj/*.o \
  -o build/FPRecorder.exe -Wl,--subsystem,windows \
  -lole32 -loleaut32 -luuid -lwinmm -lshlwapi -lcomdlg32 -luser32 -lgdi32 \
  -lshell32 -ladvapi32 -lavrt -ldwmapi -lcomctl32 -lpropsys \
  -luiautomationcore -lmfplat -lmmdevapi
```

`-municode` обязателен: точка входа — `wWinMain`. Без `-lmmdevapi` не линкуется
`ActivateAudioInterfaceAsync`, без `-lmfplat` — `MFStartup`.

## Правки в исходниках

- `main.cpp`: девять `throw std::exception("...")` заменены на
  `throw std::runtime_error(...)` — в libc++ у `std::exception` нет конструктора
  от строки (это расширение MSVC). Поведение то же, ловится тем же `catch`.
- `scratch/build/ci/stacktrace`: печатает адреса кадров, без имён функций.

## Мелочи

- `zig c++` не понимает `-mwindows`; подсистему задаёт `-Wl,--subsystem,windows`.
- Кэш Zig по умолчанию пишет в `~/.cache` и падает на rename; путь задаётся
  только через `ZIG_LOCAL_CACHE_DIR`/`ZIG_GLOBAL_CACHE_DIR`, причём абсолютный.
- Явный `-o` для объектников обязателен, иначе тот же сбой rename.
