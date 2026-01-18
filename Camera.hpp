#ifndef Camera_hpp
#define Camera_hpp

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

namespace gps {
    
    enum MOVE_DIRECTION {MOVE_FORWARD, MOVE_BACKWARD, MOVE_RIGHT, MOVE_LEFT, MOVE_UP, MOVE_DOWN};
    
    class Camera {

    public:
        //Camera constructor
        Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp);
        //return the view matrix, using the glm::lookAt() function
        glm::mat4 getViewMatrix();
        //update the camera internal parameters following a camera move event
        void move(MOVE_DIRECTION direction, float speed);
        //update the camera internal parameters following a camera rotate event
        //yaw - camera rotation around the y axis
            void setMaxHeight(float maxY);
            float getMaxHeight();
        //pitch - camera rotation around the x axis
        void rotate(float pitch, float yaw);
        glm::vec3 getPosition();
        // set movement bounds (min and max allowed camera coordinates)
        void setMovementBounds(const glm::vec3& minBound, const glm::vec3& maxBound);
        
    private:
        glm::vec3 cameraPosition;
        glm::vec3 cameraTarget;
        glm::vec3 cameraFrontDirection;
        glm::vec3 cameraRightDirection;
            float maxHeight;
        glm::vec3 minBoundary;
        glm::vec3 maxBoundary;
        glm::vec3 cameraUpDirection;
        float yaw;
        float pitch;
    };    
}

#endif /* Camera_hpp */
