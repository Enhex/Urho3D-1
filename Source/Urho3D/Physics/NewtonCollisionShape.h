#pragma once

#include "../Physics/CollisionShape.h"
#include "../Scene/Component.h"

class NewtonCollision;
class NewtonMesh;
namespace Urho3D
{



    class UrhoNewtonPhysicsWorld;
    class NewtonRigidBody;
    class NewtonMeshObject;
    class Component;
    class Model;

    /// base class for newton collision shapes
    class URHO3D_API NewtonCollisionShape : public Component
    {
        URHO3D_OBJECT(NewtonCollisionShape, Component);
    public:

        friend class UrhoNewtonPhysicsWorld;
        friend class NewtonRigidBody;

        NewtonCollisionShape(Context* context);

        virtual ~NewtonCollisionShape();

        static void RegisterObject(Context* context);

        /// Set the positional offset of the shape in local space to the node.
        void SetPositionOffset(Vector3 position) { position_ = position; MarkDirty(true); }

        /// Set the rotational offset of the shape in local space to the node.
        void SetRotationOffset(Quaternion rotation) { rotation_ = rotation; MarkDirty(true); }



        /// Returns the volume of the collision shape (convex only);
        float GetVolume() { return volume_; }

        /// Mark the shape as dirty causing it to be rebuilt by the physics world.
        void MarkDirty(bool dirty = true);
        /// Get the current dirty status.
        bool GetDirty() const { return shapeNeedsRebuilt_; }

        /// Returns the internal newton collision
        NewtonCollision* GetNewtonCollision();


    protected:

        /// Physics world.
        WeakPtr<UrhoNewtonPhysicsWorld> physicsWorld_;
        /// Rigid body.
        WeakPtr<NewtonRigidBody> rigidBody_;
        /// Internal Newton Collision
        NewtonCollision* newtonCollision_ = nullptr;
        /// newton Mesh reference
        WeakPtr<NewtonMeshObject> newtonMesh_ = nullptr;
        /// volume
        float volume_ = 0.0f;
        /// shape dirty flag
        bool shapeNeedsRebuilt_ = true;

        /// Offset position.
        Vector3 position_;
        /// Offset rotation.
        Quaternion rotation_;


        /// updates the intenal newton collision pointer to reference the appropriate collision instance from the newton cache based on current parameters.
        void reEvaluateCollision();
        /// implement this in subclasses to create the internal newton collision
        virtual void createNewtonCollision();
        /// Frees the internal collision shape and mesh;
        void freeInternalCollision();

        /// notifies the sibling rigid body of updates if it exists
        void notifyRigidBody();

        /// Called when there is a change to the rigid body component;
        void updateReferenceToRigidBody();

        /// Calculates the effective mass based off density and size. (could be expensive)
        float updateVolume();



        virtual void OnNodeSet(Node* node) override;
    };







    class URHO3D_API NewtonCollisionShape_Box : public NewtonCollisionShape {

        URHO3D_OBJECT(NewtonCollisionShape_Box, NewtonCollisionShape);

    public:
        NewtonCollisionShape_Box(Context* context);
        virtual ~NewtonCollisionShape_Box();

        static void RegisterObject(Context* context);

        /// Set the size of the box
        void SetSize(const Vector3& size) { size_ = size; MarkDirty(true); }

        /// Get the size of the box
        Vector3 GetSize() const { return size_; }

    protected:
        Vector3 size_ = Vector3::ONE;

        virtual void createNewtonCollision() override;

    };

    class URHO3D_API NewtonCollisionShape_Sphere : public NewtonCollisionShape {

        URHO3D_OBJECT(NewtonCollisionShape_Sphere, NewtonCollisionShape);

    public:
        NewtonCollisionShape_Sphere(Context* context);
        virtual ~NewtonCollisionShape_Sphere();

        static void RegisterObject(Context* context);


        void SetRadius(float radius) { radius_ = radius; MarkDirty(true); }


        float GetRadius() const { return radius_; }

    protected:
        float radius_ = 0.5f;

        virtual void createNewtonCollision() override;

    };

    class URHO3D_API NewtonCollisionShape_Capsule : public NewtonCollisionShape {

        URHO3D_OBJECT(NewtonCollisionShape_Capsule, NewtonCollisionShape);

    public:
        NewtonCollisionShape_Capsule(Context* context);
        virtual ~NewtonCollisionShape_Capsule();

        static void RegisterObject(Context* context);


        void SetRadius1(float radius) { radius1_ = radius; MarkDirty(true); }
        float GetRadius1() const { return radius1_; }


        void SetRadius2(float radius) { radius2_ = radius; MarkDirty(true); }
        float GetRadius2() const { return radius2_; }

    protected:
        float length_ = 1.0f;
        float radius1_ = 0.5f;
        float radius2_ = 0.5f;

        virtual void createNewtonCollision() override;

    };

    class URHO3D_API NewtonCollisionShape_Cone : public NewtonCollisionShape {

        URHO3D_OBJECT(NewtonCollisionShape_Cone, NewtonCollisionShape);

    public:
        NewtonCollisionShape_Cone(Context* context);
        virtual ~NewtonCollisionShape_Cone();

        static void RegisterObject(Context* context);

        void SetRadius(float radius) { radius_ = radius; MarkDirty(true); }
        float GetRadius() const { return radius_; }


    protected:
        float length_ = 1.0f;
        float radius_ = 0.5f;


        virtual void createNewtonCollision() override;

    };


    class URHO3D_API NewtonCollisionShape_Cylinder : public NewtonCollisionShape {

        URHO3D_OBJECT(NewtonCollisionShape_Cylinder, NewtonCollisionShape);

    public:
        NewtonCollisionShape_Cylinder(Context* context);
        virtual ~NewtonCollisionShape_Cylinder();

        static void RegisterObject(Context* context);

        /// Set Radius in 1st dimension
        void SetRadius1(float radius) { radius1_ = radius; MarkDirty(true); }

        float GetRadius1() const { return radius1_; }

        /// Set Radius in 2nd dimension
        void SetRadius2(float radius) { radius2_ = radius; MarkDirty(true); }

        float GetRadius2() const { return radius2_; }



        void SetLength(float length) { length_ = length; MarkDirty(true); }

        float GetLength() const { return length_; }

    protected:
        float radius1_ = 0.5f;
        float radius2_ = 0.5f;
        float length_ = 1.0f;

        virtual void createNewtonCollision() override;

    };




    class URHO3D_API NewtonCollisionShape_Geometry : public NewtonCollisionShape {

        URHO3D_OBJECT(NewtonCollisionShape_Geometry, NewtonCollisionShape);

    public:
        NewtonCollisionShape_Geometry(Context* context);
        virtual ~NewtonCollisionShape_Geometry();

        static void RegisterObject(Context* context);

        /// Set model to create geometry from
        void SetModel(Model* model) { model_ = model; MarkDirty(); }

        Model* GetModel() const { return model_; }

        /// Set the tolerancing for hull creation.
        void SetHullTolerance(float tolerance = 0.0f) { hullTolerance_ = 0.0f; MarkDirty(); }

    protected:
        /// optional Model reference
        WeakPtr<Model> model_;
        /// lod level
        unsigned modelLodLevel_ = 0;
        /// model geometry index to use
        unsigned modelGeomIndx_ = 0;
        /// Hulling tolerance
        unsigned hullTolerance_ = 0.0f;

        virtual void createNewtonCollision() override;

        void autoSetModel();

        ///forms newtonMesh_ from model geometry for later use.
        bool resolveOrCreateTriangleMesh();

        virtual void OnNodeSet(Node* node) override;

    };

    class URHO3D_API NewtonCollisionShape_ConvexHullCompound : public NewtonCollisionShape_Geometry {

        URHO3D_OBJECT(NewtonCollisionShape_ConvexHullCompound, NewtonCollisionShape_Geometry);

    public:
        NewtonCollisionShape_ConvexHullCompound(Context* context);
        virtual ~NewtonCollisionShape_ConvexHullCompound();

        static void RegisterObject(Context* context);

    protected:

        virtual void createNewtonCollision() override;

    };

    class URHO3D_API NewtonCollisionShape_ConvexHull : public NewtonCollisionShape_Geometry {

        URHO3D_OBJECT(NewtonCollisionShape_ConvexHull, NewtonCollisionShape_Geometry);

    public:
        NewtonCollisionShape_ConvexHull(Context* context);
        virtual ~NewtonCollisionShape_ConvexHull();

        static void RegisterObject(Context* context);

    protected:

        virtual void createNewtonCollision() override;

    };


}
