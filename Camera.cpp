#include "Camera.hpp"
#include <cmath>

namespace gps {

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = cameraUp;

        // initialize front and right directions
        this->cameraFrontDirection = glm::normalize(this->cameraTarget - this->cameraPosition);
        this->cameraRightDirection = glm::normalize(glm::cross(this->cameraFrontDirection, this->cameraUpDirection));
        // initialize yaw/pitch from front vector
        this->yaw = glm::degrees(std::atan2(this->cameraFrontDirection.z, this->cameraFrontDirection.x));
        this->pitch = glm::degrees(std::asin(this->cameraFrontDirection.y));
        this->maxHeight = 1000.0f; // default very high
        // default movement bounds (very large by default)
        this->minBoundary = glm::vec3(-10000.0f);
        this->maxBoundary = glm::vec3(10000.0f);
        
    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        //TODO

        return glm::lookAt(cameraPosition, cameraTarget, this->cameraUpDirection);
    }

    glm::vec3 Camera::getPosition() {
        return this->cameraPosition;
    }

    void Camera::setPosition(const glm::vec3& position) {
        this->cameraPosition = position;
        // keep target consistent with front direction
        this->cameraFrontDirection = glm::normalize(this->cameraTarget - this->cameraPosition);
        this->cameraRightDirection = glm::normalize(glm::cross(this->cameraFrontDirection, this->cameraUpDirection));
        // recompute yaw/pitch from front
        this->yaw = glm::degrees(std::atan2(this->cameraFrontDirection.z, this->cameraFrontDirection.x));
        this->pitch = glm::degrees(std::asin(this->cameraFrontDirection.y));
    }

    void Camera::setTarget(const glm::vec3& target) {
        this->cameraTarget = target;
        this->cameraFrontDirection = glm::normalize(this->cameraTarget - this->cameraPosition);
        this->cameraRightDirection = glm::normalize(glm::cross(this->cameraFrontDirection, this->cameraUpDirection));
        this->yaw = glm::degrees(std::atan2(this->cameraFrontDirection.z, this->cameraFrontDirection.x));
        this->pitch = glm::degrees(std::asin(this->cameraFrontDirection.y));
    }

    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        if (direction == MOVE_FORWARD) {
            this->cameraPosition += this->cameraFrontDirection * speed;
            this->cameraTarget += this->cameraFrontDirection * speed;
        } else if (direction == MOVE_BACKWARD) {
            this->cameraPosition -= this->cameraFrontDirection * speed;
            this->cameraTarget -= this->cameraFrontDirection * speed;
        } else if (direction == MOVE_RIGHT) {
            this->cameraPosition += this->cameraRightDirection * speed;
            this->cameraTarget += this->cameraRightDirection * speed;
        } else if (direction == MOVE_LEFT) {
            this->cameraPosition -= this->cameraRightDirection * speed;
            this->cameraTarget -= this->cameraRightDirection * speed;
        } else if (direction == MOVE_UP) {
            glm::vec3 newPos = this->cameraPosition + this->cameraUpDirection * speed;
            if (newPos.y > this->maxHeight) newPos.y = this->maxHeight;
            glm::vec3 delta = newPos - this->cameraPosition;
            this->cameraPosition += delta;
            this->cameraTarget += delta;
        } else if (direction == MOVE_DOWN) {
            this->cameraPosition -= this->cameraUpDirection * speed;
            this->cameraTarget -= this->cameraUpDirection * speed;
        }
        // clamp position to movement bounds
        this->cameraPosition.x = glm::clamp(this->cameraPosition.x, this->minBoundary.x, this->maxBoundary.x);
        this->cameraPosition.y = glm::clamp(this->cameraPosition.y, this->minBoundary.y, this->maxBoundary.y);
        this->cameraPosition.z = glm::clamp(this->cameraPosition.z, this->minBoundary.z, this->maxBoundary.z);
        // keep target consistent with front direction after clamping
        this->cameraTarget = this->cameraPosition + this->cameraFrontDirection;
    }

    void Camera::setMaxHeight(float maxY) {
        this->maxHeight = maxY;
    }

    void Camera::setMovementBounds(const glm::vec3& minBound, const glm::vec3& maxBound) {
        this->minBoundary = minBound;
        this->maxBoundary = maxBound;
    }

    float Camera::getMaxHeight() {
        return this->maxHeight;
    }

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    //pitch - camera rotation around the x axis
    void Camera::rotate(float pitch, float yaw) {
        // adjust angles
        this->pitch += pitch;
        this->yaw += yaw;

        // constrain pitch
        if (this->pitch > 89.0f) this->pitch = 89.0f;
        if (this->pitch < -89.0f) this->pitch = -89.0f;

        // recalculate front vector
        glm::vec3 front;
        front.x = cos(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
        front.y = sin(glm::radians(this->pitch));
        front.z = sin(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
        this->cameraFrontDirection = glm::normalize(front);
        this->cameraRightDirection = glm::normalize(glm::cross(this->cameraFrontDirection, this->cameraUpDirection));
        this->cameraTarget = this->cameraPosition + this->cameraFrontDirection;
    }
}
