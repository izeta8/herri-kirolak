---
name: calibrar-gesto
description: Diseña o ajusta un gesto del Wii Remote con datos reales (telemetría, grabación guiada, análisis de rasgos, réplica del detector). Usar cuando un gesto no se detecta bien o al crear el gesto de un deporte nuevo.
---

# Calibrar un gesto con datos reales

Leer antes `docs/GESTOS.md` (qué mide y qué no el mando, y lo aprendido con el hachazo).

1. **Gesto definido**: confirmar con el usuario agarre + fases por escrito. Apuntarlo en `docs/DECISIONES.md`.
2. **Grabar**:
   - Mandar a la Wii la pantalla de prueba del gesto (skill `mandar-a-wii`, versión con `START_SCENE` de esa pantalla).
   - Receptor de telemetría en segundo plano y comprobar que llegan datos (`tail build/telemetria.log`).
   - Pedir al usuario: botón 1 → grabación guiada, ~30 gestos a ritmo de juego, botón 1 para acabar.
   - Copiar la sesión desde su `# conexion` a `tuning-data/AAAA-MM-DD-<qué>.csv` (las reconexiones parten la sesión:
     incluir todos los tramos).
3. **Analizar** (scripts en `tools/`, o uno ad hoc en el scratchpad):
   - ¿Se detectan todos? (`replay_detector.py`). Si no, seguir a mano un gesto fallido lectura a lectura
     (|a|, ax/ay/az, ángulo, estado) y ver qué condición no se cumple.
   - Tiempos: inicio → pico → fin (ms). La respuesta del juego debe coincidir con el pico.
   - Separar clases: rasgos por gesto → Fisher por rasgo → LDA con 2–4 rasgos → **validación dejando uno fuera**.
4. **Implementar** en `src/engine/input.cpp` (+ constantes en `tuning.h`) y **la misma lógica** en
   `tools/replay_detector.py`.
5. **Verificar** con la réplica sobre todos los `tuning-data/` relevantes; anotar aciertos.
6. Compilar, probar CPU en Dolphin, mandar en caliente y que el usuario lo pruebe jugando.
7. Documentar: `docs/GESTOS.md` (detector, constantes, aciertos, datos nuevos), `CLAUDE.md` si cambia algo de uso.
