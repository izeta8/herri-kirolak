---
name: nuevo-juego
description: Crea un deporte nuevo de Herri Kirolak (harri-jasotzea, sokatira, trontza…) reutilizando el motor, personajes, plaza y UI, con entrenamiento y lehiaketa a la vez. Usar cuando el usuario quiera empezar otro juego.
---

# Crear un deporte nuevo

Seguir `docs/NUEVO-JUEGO.md`. Resumen del orden:

1. **Diseño con el usuario** (antes de programar): cómo es el deporte real, el gesto con el Wii Remote (agarre + fases),
   qué son las rondas, cómo se ve la lehiaketa a la vez. Escribirlo en `docs/SPEC.md` y las decisiones en
   `docs/DECISIONES.md`. Proponer, no imponer: el usuario decide.
2. **Esqueleto jugable con la CPU**: `src/games/<nombre>/` (objetos, rig del personaje, escena con `Player` por jugador),
   registro en `src/game/sports.cpp`, fábrica en `scenes.h`, textos en `text.h`, constantes en `tuning.h`,
   carpeta en `SOURCES` del Makefile, escenas en `START_SCENE`. Probar con la skill `probar-en-dolphin`.
3. **Gesto**: detector en `input.cpp` + réplica Python; pantalla de prueba del gesto (como Kolpe proba) en el registro.
   Calibrar con la skill `calibrar-gesto`.
4. **Sensación** (como el hachazo): respuesta en el pico del gesto, pausa de impacto, temblor, partículas, vibración,
   tarjeta "Nola jo", indicadores grandes (sin textos que roten).
5. **Lehiaketa** 1–4 a la vez + resultados + récord (ampliar `save.dat` a récords por deporte si aún no está).
6. Wii real (skill `mandar-a-wii`), iterar con el usuario.
7. Checklist de `docs/NUEVO-JUEGO.md` y actualizar ROADMAP, README (captura) y el resto de docs.
