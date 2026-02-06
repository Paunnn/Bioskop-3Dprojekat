#pragma once

#include "glm/glm.hpp"

enum CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float FOV = 45.0f;

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;

    float MovementSpeed;
    float MouseSensitivity;
    float Fov;

    glm::vec3 RoomMin;
    glm::vec3 RoomMax;
    float PlayerHeight;
    float PlayerRadius;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = YAW, float pitch = PITCH)
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)),
          MovementSpeed(SPEED),
          MouseSensitivity(SENSITIVITY),
          Fov(FOV),
          PlayerHeight(1.7f),
          PlayerRadius(0.3f)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;

        RoomMin = glm::vec3(-10.0f, 0.1f, -7.5f);
        RoomMax = glm::vec3(10.0f, 10.0f, 7.5f);

        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void processKeyboard(CameraMovement direction, float deltaTime) {
        float velocity = MovementSpeed * deltaTime;
        glm::vec3 newPos = Position;

        glm::vec3 rightDir = glm::normalize(glm::cross(Front, WorldUp));

        if (direction == FORWARD)
            newPos = newPos + Front * velocity;
        if (direction == BACKWARD)
            newPos = newPos - Front * velocity;
        if (direction == LEFT)
            newPos = newPos - rightDir * velocity;
        if (direction == RIGHT)
            newPos = newPos + rightDir * velocity;

        newPos = constrainToBounds(newPos);
        Position = newPos;
    }

    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        if (constrainPitch) {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        updateCameraVectors();
    }

    void setRoomBounds(glm::vec3 min, glm::vec3 max) {
        RoomMin = min;
        RoomMax = max;
    }

    glm::vec3 getRayDirection() {
        return Front;
    }

private:
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cosf(glm::radians(Yaw)) * cosf(glm::radians(Pitch));
        front.y = sinf(glm::radians(Pitch));
        front.z = sinf(glm::radians(Yaw)) * cosf(glm::radians(Pitch));
        Front = glm::normalize(front);

        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }

    glm::vec3 constrainToBounds(glm::vec3 pos) {
        pos.x = glm::clamp(pos.x, RoomMin.x + PlayerRadius, RoomMax.x - PlayerRadius);
        pos.y = glm::clamp(pos.y, RoomMin.y + PlayerHeight, RoomMax.y - 0.3f);
        pos.z = glm::clamp(pos.z, RoomMin.z + PlayerRadius, RoomMax.z - PlayerRadius);
        return pos;
    }
};
