//
//  MovableObject.cpp
//  SimulatorSFML
//
//  Created by Samuel Arbibe on 22/11/2019.
//  Copyright © 2019 Samuel Arbibe. All rights reserved.
//

#include "Vehicle.hpp"

const Vector2f m_forwardVec{0.f, -1.f};
int         Vehicle::toBeDeleted{0};
VehicleType Vehicle::Car{CAR, "Car", "../../resources/Cars/car_image_", 5,  Vector2f(0.6f, 0.6f)};
VehicleType Vehicle::Truck{TRUCK, "Truck",  "../../resources/Cars/car_image_",5, Vector2f(0.12f, 0.12f)};
VehicleType Vehicle::Motorcycle{MOTORCYCLE,  "Motorcycle", "../resources/Cars/motorcycle_image_",5, Vector2f(0.12f, 0.12f)};

Vehicle::Vehicle(VehicleTypeOptions vehicleType, int vehicleNumber, Lane * sourceLane, Lane * destinationLane, Intersection * currentIntersection)
{
    // set initial values for the movable object
    m_vehicleType    = GetVehicleTypeByOption(vehicleType);
    m_vehicleNumber  = vehicleNumber;
    m_speed          = 150.f;
    m_maxAcceleration= m_vehicleType->MaxAcceleration;
    m_maxSpeed       = m_vehicleType->MaxSpeed;
    m_state          = DRIVE;
    m_sourceLane     = sourceLane;
    m_targetLane     = destinationLane;
    m_currentIntersection   = currentIntersection;
    m_currentLane    = m_sourceLane;
    m_angularV      = 0;

    m_rotation       = sourceLane->GetDirection();
    m_position       = sourceLane->GetStartPosition();
    m_vehicleInFront = nullptr;

    // if vehicle texture hasn't been loaded yet, load it
    Vehicle::LoadVehicleTextures(m_vehicleType);

    // set up sprite
    int textureNumber = (m_vehicleNumber % m_vehicleType->ImageCount);

    m_texture = &(m_vehicleType->Textures->at(textureNumber));
    //m_sprite.setColor(Color::Red);
    m_sprite.setTexture(*m_texture);

    m_sprite.setScale(m_vehicleType->Scale);
    m_sprite.setOrigin(m_sprite.getTextureRect().width/2, m_sprite.getTextureRect().height/3);
}

void Vehicle::ClearVehicles()
{
    auto it = ActiveVehicles.begin();

    // while there are cars to delete;
    while(toBeDeleted != 0 && it != ActiveVehicles.end())
    {
        // if is to be deleted
        if((*it)->GetState() == DELETE)
        {
            Vehicle * temp = (*it);
            it = ActiveVehicles.erase(it);

            delete temp;

            toBeDeleted--;
            cout << "active vehicles : " << ActiveVehicles.size() << endl;
        }
        else
        {
            it++;
        }
    }
}

Vehicle * Vehicle::AddVehicle(Lane * sourceLane, Lane * destinationLane, Intersection * currentIntersection,
                              VehicleTypeOptions vehicleType, int vehicleNumber){
    //add vehicle to vector
    //if(vehicleNumber == 0)vehicleNumber = VehicleCount + 1;
    auto * temp = new Vehicle(vehicleType, vehicleNumber, sourceLane, destinationLane, currentIntersection);
    ActiveVehicles.push_back(temp);

    temp->m_vehicleInFront = (sourceLane->GetLastCar()) ? GetVehicle(sourceLane->GetLastCar()) : nullptr;

    //set this car as the last car that entered the lane
    sourceLane->SetLastCar(vehicleNumber);
    sourceLane->AddVehicleCount();
    VehicleCount++;

    cout << "car " << vehicleNumber << " added to lane " << sourceLane->GetLaneNumber() << endl;

    return temp;
}

void Vehicle::LoadVehicleTextures(VehicleType * vehicleType)
{
    if(vehicleType->Textures == nullptr)
    {
        vehicleType->Textures = new vector<Texture>();

        string directory;
        Texture tempTexture;
        for (int i = 1; i <= vehicleType->ImageCount; ++i)
        {
            directory = vehicleType->ImageDir + to_string(i) + ".png";

            if (tempTexture.loadFromFile(directory))
            {
                tempTexture.setSmooth(true);
                vehicleType->Textures->push_back(tempTexture);
            }
            else
            {
                cout << "loading texture no." << i << " for " << vehicleType->VehicleTypeName << endl;
            }
        }
    }
}

void Vehicle::SetMaxSpeed(VehicleTypeOptions vehicleType, float max_speed)
{    VehicleType * temp = GetVehicleTypeByOption(vehicleType);
    temp->MaxSpeed = max_speed;
    temp->MaxAcceleration = max_speed/3.f;
}

VehicleType * Vehicle::GetVehicleTypeByOption(VehicleTypeOptions vehicleTypeOptions)
{
    switch (vehicleTypeOptions)
    {

        case TRUCK:
            return &(Vehicle::Truck);
        case MOTORCYCLE:
            return &(Vehicle::Motorcycle);
        case CAR:
        default:
            return &(Vehicle::Car);
    }
}

Vehicle * Vehicle::GetVehicle(int vehicleNumber)
{
    for(Vehicle *v : ActiveVehicles)
    {
        if(v->m_vehicleNumber == vehicleNumber)
        {
            return v;
        }
    }

    return nullptr;
}

void Vehicle::TransferVehicle(Vehicle * vehicle, Lane * fromLane, Lane * toLane)
{
    if(vehicle == nullptr)
    {
        cout << "vehicle not found" << endl;
        return;
    }

    if(fromLane != nullptr)
    {
        if(vehicle->m_currentLane->GetLaneNumber() != fromLane->GetLaneNumber())
        {
            cout << "vehicle is not in the given lane" << endl;
            return;
        }
        fromLane->RemoveVehicleCount();
    }

    vehicle->m_currentLane    = toLane;
    vehicle->m_rotation       = m_currentLane->GetDirection();
    vehicle->m_angularV       = 0;
    vehicle->m_speed          = m_speed;
    vehicle->m_vehicleInFront = (m_currentLane->GetLastCar()) ? GetVehicle(m_currentLane->GetLastCar()) : nullptr;
    vehicle->m_currentLane->SetLastCar(vehicle->m_vehicleNumber);
}


State Vehicle::drive()
{

    // check for distance with car in front
    if(m_vehicleInFront != nullptr && m_vehicleInFront->m_state != DELETE)
    {
        float distanceFromNextCar = calculateDistance(m_position, m_vehicleInFront->m_position);

        if(distanceFromNextCar < 200)
        {
            m_state = STOP;

            if(distanceFromNextCar < 50 || m_speed < 10)
            {
                m_acceleration = 0;
                m_speed = 0;
                return m_state;
            }
            else
            {
                if(m_vehicleInFront->m_acceleration < 0 || distanceFromNextCar < 200){
                    // using v^2 = v0^2 + 2a(x-x0)
                    m_acceleration = ((m_speed*m_speed)/(2*(150.f - distanceFromNextCar)));
                    return m_state;
                }
            }
        }
    }

    //TODO: find better algorithm

    // check if car is in between lanes
    if(m_currentIntersection->getGlobalBounds().contains(m_position))
    {
        m_currentLane = nullptr;
        m_state = TURN;

        if(m_angularV == 0)
        {
            float distanceSourceTarget = calculateDistance(m_sourceLane->GetEndPosition(), m_targetLane->GetStartPosition());

            float angle = (m_sourceLane->GetDirection() - m_targetLane->GetDirection());

            float turningRadius = (distanceSourceTarget/2.f) / (sin(angle * M_PI/360.f));

            float turningParameter = 2.f * M_PI * turningRadius;

            float turningDistance = (angle/360.f) * turningParameter;

            m_angularV = -angle/turningDistance;
        }
        //set rotation
        return TURN;
    }

    // check distance from stop (if lane is blocked)
    if(m_currentLane != nullptr && m_currentLane != m_targetLane && m_currentLane->GetIsBlocked())
    {
        float distanceFromStop = calculateDistance(this->m_position, m_currentLane->GetEndPosition());

        if(distanceFromStop < 150)
        {
            m_state = STOP;

            if(distanceFromStop < 50 || m_speed < 5)
            {
                m_speed = 0;
            }
            else
            {
                // using v^2 = v0^2 + 2a(x-x0)
                m_acceleration = ((m_speed*m_speed)/(2*(50 - distanceFromStop)));
            }
            return m_state;
        }

        return m_state;
    }

    // check if car is in targetLane
    if(m_currentLane != m_targetLane && m_targetLane->getGlobalBounds().contains(m_position))
    {
        // we need to transfer vehicle to target lane
        TransferVehicle(this, nullptr, m_targetLane);

        m_acceleration = m_maxAcceleration;
        m_state = DRIVE;
        return DRIVE;
    }

    // check if car is no longer in intersection
    if(m_currentLane != nullptr &&
       !m_currentIntersection->getGlobalBounds().contains(m_position) &&
       !m_currentLane->getGlobalBounds().contains(m_position))
    {
        toBeDeleted++;

        m_state = DELETE;
        return DELETE;
    }

    m_acceleration = m_maxAcceleration;
    m_state = DRIVE;
    return DRIVE;
}

float Vehicle::calculateDistance(Vector2f a, Vector2f b)
{
    float xDist = abs(a.x - b.x);
    float yDist = abs(a.y - b.y);

    return sqrt(xDist*xDist + yDist*yDist);
}

void Vehicle::Update(float elapsedTime){

    drive();
    applyChanges(elapsedTime);
}

void Vehicle::applyChanges(float elapsedTime)
{
    // apply acceleration
    m_speed += m_acceleration * elapsedTime;

    // apply max speed limit
    if(m_speed > m_maxSpeed) m_speed = m_maxSpeed;

    // apply min speed limit
    if(m_speed < 0) m_speed = 0;

    // set rotation relative to current speed, to create a constant turning radius
    Transform t;

    m_rotation += m_angularV * elapsedTime * m_speed;

    t.rotate(m_rotation);

    // rotate the movement vector in relation to the forward vector (0,1)
    m_movementVec = t.transformPoint(m_forwardVec);

    // apply movement vector on position, relative to elapsed time to ensure
    // a constant speed at any FPS
    m_position += m_movementVec * m_speed * elapsedTime;

    // apply rotation and position changes to the actual car sprite
    m_sprite.setPosition(m_position);
    m_sprite.setRotation(m_rotation);
}

void Vehicle::Draw(RenderWindow *window)
{
    (*window).draw(this->m_sprite);
}
