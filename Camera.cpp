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
        
    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        //TODO

        return glm::lookAt(cameraPosition, cameraTarget, this->cameraUpDirection);
    }

    glm::vec3 Camera::getPosition() {
        return this->cameraPosition;
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
            this->cameraPosition += this->cameraUpDirection * speed;
            this->cameraTarget += this->cameraUpDirection * speed;
        } else if (direction == MOVE_DOWN) {
            this->cameraPosition -= this->cameraUpDirection * speed;
            this->cameraTarget -= this->cameraUpDirection * speed;
        }
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
