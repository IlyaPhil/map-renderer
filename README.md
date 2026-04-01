# S57 Chart Viewer

Десктопное приложение для просмотра морских навигационных карт в формате S-57 (IHO).  
Реализовано на C++14 с использованием Qt5 и GDAL.

---

## Функциональность

### Отображение карты
- Загрузка файлов S-57 (`.000`)
- Морской фон (голубой) с цветовым разделением суши и воды
- Градиентная окраска глубинных зон (`DEPARE`) по минимальной глубине:
  - светло-голубой → тёмно-синий (от мелководья к большим глубинам)

### Слои с фильтрацией
Слои можно включать и выключать независимо в панели справа:

| Слой | S57-классы |
|------|-----------|
| Береговая линия | `COALNE`, `LNDARE`, `SLCONS` |
| Береговые объекты | `LNDMRK`, `BUAARE`, `BRIDGE`, `MORFAC`, `HULKES`, `HRBARE`, `DOCARE`, `PRTARE` |
| Створные знаки | `BCNLAT`, `BCNCAR`, `BCNISD`, `BCNSAW`, `BCNSPP`, `LIGHTS` |
| Буи | `BOYLAT`, `BOYCAR`, `BOYISD`, `BOYSAW`, `BOYSPP`, `BOYINB` |
| Глубины | `SOUNDG`, `DEPARE`, `DEPCNT` |
| Фарватер | `FAIRWY`, `RECTRC`, `TSSLPT`, `TSSRON` |
| Створные линии | `NAVLNE`, `TRSLNE` |
| Названия объектов | атрибут `OBJNAM` всех точечных объектов |

Панель слоёв содержит легенду — цветной квадрат рядом с каждым чекбоксом.

### Навигация
| Действие | Управление |
|----------|-----------|
| Перемещение карты | Зажать ЛКМ и тянуть |
| Приближение | Колесо вверх |
| Удаление | Колесо вниз |
| Приближение ×2 | Двойной клик ЛКМ |
| Показать всю карту | Кнопка «Показать всё» |

### Статусная строка
Отображает текущие координаты курсора в десятичных градусах (долгота / широта).

---

## Архитектура

```
src/
├── main.cpp          — точка входа, диалог выбора файла
├── MainWindow.h/cpp  — главное окно (QMainWindow): панель слоёв, статусбар
├── MapWidget.h/cpp   — виджет отрисовки карты (QPainter + mouse/wheel events)
├── S57Loader.h/cpp   — загрузка S57 через GDAL OGR, классификация по слоям
└── MapFeature.h      — структуры данных (MapFeature, LayerType)
```

### Поток данных
```
Файл .000
   ↓
S57Loader::load()        ← GDAL OGR (каждый S57-класс = отдельный слой)
   ↓
QVector<MapFeature>      ← геометрия + атрибуты + LayerType
   ↓
MapWidget::setFeatures() ← хранит features, вызывает fitAll()
   ↓
MapWidget::paintEvent()  ← QPainter: Area → Line → Point (с учётом z-порядка слоёв)
```

### Координатное преобразование
S57 хранит координаты в десятичных градусах WGS84.  
`MapWidget` использует линейное преобразование с сохранением пропорций:

```
screenX =  lon * scale + panX
screenY = -lat * scale + panY   // ось Y перевёрнута
```

Параметры `scale` и `pan` обновляются при pan/zoom/fitAll.

---

## Зависимости

| Библиотека | Версия | Назначение |
|-----------|--------|-----------|
| Qt5 | 5.x | GUI, отрисовка, события |
| GDAL | 3.x | Чтение S57, геопространственные данные |
| GCC / MinGW | 15.x | Компилятор C++14 |

---

## Установка окружения (Windows, MSYS2)

### 1. Установить MSYS2
Скачать установщик с [msys2.org](https://www.msys2.org/), установить в `C:\msys64`.

### 2. Обновить базы пакетов
Открыть **MSYS2 MSYS** и выполнить:
```bash
pacman -Syu
```
Закрыть терминал, открыть снова и повторить.

### 3. Установить зависимости
Открыть **MSYS2 MinGW64** и выполнить:
```bash
pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-gdal mingw-w64-x86_64-qt5-base \
          mingw-w64-x86_64-qt5-tools
```

### 4. Настроить VSCode
- Установить расширения: **C/C++** и **CMake Tools** (Microsoft)
- В настройках VSCode (`Ctrl+,`) указать путь к CMake:
  ```
  cmake.cmakePath: C:/msys64/mingw64/bin/cmake.exe
  ```
- В **CMake Tools → Edit User-Local CMake Kits** добавить кит:
  ```json
  [{
    "name": "MSYS2 MinGW64 GCC",
    "compilers": {
      "C":   "C:/msys64/mingw64/bin/gcc.exe",
      "CXX": "C:/msys64/mingw64/bin/g++.exe"
    },
    "generator": "MinGW Makefiles",
    "environmentVariables": {
      "PATH": "C:/msys64/mingw64/bin;${env:PATH}"
    }
  }]
  ```

---

## Сборка и запуск

### Через VSCode
1. `Ctrl+Shift+P` → **CMake: Configure**
2. `Ctrl+Shift+P` → **CMake: Build**
3. `Ctrl+Shift+P` → **CMake: Run Without Debugging**

### Через терминал (MSYS2 MinGW64)
```bash
cd /e/map-renderer
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
./MapRenderer.exe "../S57 viewer/Карты формата S57/1V620101.000"
```

При запуске без аргумента откроется диалог выбора файла.

---

## Известные ограничения
- Приложение работает автономно, без подключения к интернету
- Поддерживаются только файлы стандарта IHO S-57 Edition 3.1 (`.000`)
- Глубины на промерах (`SOUNDG`) отображаются только при достаточном масштабе