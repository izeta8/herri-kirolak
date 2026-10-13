#pragma once
// El aizkolari: personaje subido al tronco con su hacha y sus animaciones.
#include "game/character.h"

class Log;

class Aizkolari {
public:
    enum Phase { IDLE, READY, SWING, STUCK, RECOVER, TURN, CELEBRATE };

    void setup(const Appearance& look, const Log* log);
    void reset();  // de pie sobre el tronco, mirando hacia delante (+Z)
    void update(float dt);
    void draw();

    void setReady();
    void setIdle();
    bool chop(float strength, float tilt);  // false si no puede golpear ahora (se guarda uno en cola)
    // El jugador empieza a bajar: el hacha baja ya y espera en la madera a que llegue chop()
    void startSwing();
    void cancelSwing();  // la bajada no fue hachazo: vuelve a subir sin cortar
    void turnAround();
    void celebrate();

    Phase phase() const { return phase_; }
    int side() const { return side_; }  // 0 = corta por delante, 1 = por detrás
    bool turning() const { return phase_ == TURN; }
    bool busy() const { return phase_ == SWING || phase_ == STUCK; }
    // Evento: este frame el hacha ha llegado a la madera
    bool impact() const { return impact_; }
    float impactStrength() const { return impStrength_; }
    float impactTilt() const { return impTilt_; }
    Vec3 axeHead() const { return axeHead_; }
    Vec3 feet() const { return feet_; }
    float yaw() const { return yaw_; }
    void setLiveTilt(float t) { liveTilt_ = t; }
    // Altura del hacha en reposo (0 = delante, 1 = en alto); el jugador la controla con el mando
    void setRaise(float r) { raise_ = r; }
    float fatigueLook = 0;  // 0..1, para animar el cansancio

private:
    void computePose(CharacterPose& pose, Xf& axeXf);
    float impactAngle();

    CharacterModel* model_ = nullptr;
    const Log* log_ = nullptr;
    Phase phase_ = IDLE;
    float t_ = 0;
    float phi_ = 0.3f, phiFrom_ = 0;
    float tilt_ = 0, liveTilt_ = 0, raise_ = 1.0f;
    float readyPhi() const;
    float yaw_ = 0, yawFrom_ = 0;
    int side_ = 0;
    Vec3 feet_;
    float hop_ = 0;
    bool impact_ = false;
    float strength_ = 0, chopTilt_ = 0, impStrength_ = 0, impTilt_ = 0;
    bool queued_ = false;
    bool resolved_ = true;  // en SWING: ya se sabe fuerza y lado del golpe
    float resolvedAt_ = -1; // t_ en el que llegó el resultado (-1 = se sabía desde el principio)
    float phiResolve_ = 0;
    float qStrength_ = 0, qTilt_ = 0;
    float blinkT_ = 2;
    float time_ = 0;
    Vec3 axeHead_;
};
