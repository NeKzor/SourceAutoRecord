#pragma once
#include <map>
#include <sstream>

#include "Feature.hpp"

#include "Utils/SDK.hpp"
#include "Variable.hpp"
#include "Modules/Engine.hpp"


extern Variable sar_cam_control;
extern Variable sar_cam_drive;

extern Variable cl_skip_player_render_in_main_view;
extern Variable r_drawviewmodel;
extern Variable ss_force_primary_fullscreen;
extern Variable crosshair;

struct CameraState {
    Vector origin = Vector();
    QAngle angles = QAngle();
    float fov = 90;
    operator std::string() const{
        std::ostringstream s;
        s << "pos: " << origin.x << " " << origin.y << " " << origin.z << "; ";
        s << "rot: " << angles.x << " " << angles.y << " " << angles.z << "; ";
        s << "fov: " << fov;
        return s.str();
    }
};

enum CameraStateParameter {
    ORIGIN_X,
    ORIGIN_Y,
    ORIGIN_Z,
    ANGLES_X,
    ANGLES_Y,
    ANGLES_Z,
    FOV
};

enum CameraControlType {
    Default,
    Drive,
    Cinematic,
};

class Camera : public Feature {
private:
    bool manualActive = false;
    bool cameraRefreshRequested = false;
    bool timeOffsetRefreshRequested = true;
    int mouseHoldPos[2] = {0,0};
    float timeOffset = 0.0;
public:
    CameraControlType controlType = Default;
    CameraState currentState;
    std::map<int, CameraState> states;
    Camera();
    ~Camera();
    bool IsDriving();
    void OverrideView(CPortalViewSetup1* m_View);
    CameraState InterpolateStates(float time);
    void RequestTimeOffsetRefresh();
    void RequestCameraRefresh();
    void OverrideMovement(CUserCmd* cmd);
};

extern Camera* camera;

extern DECL_DECLARE_AUTOCOMPLETION_FUNCTION(sar_cam_path_setkf);