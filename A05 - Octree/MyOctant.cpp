#include "MyOctant.h"

namespace Simplex
{
	uint MyOctant::m_uOctantCount = 0;
	uint MyOctant::m_uMaxLevel = 2;
	uint MyOctant::m_uIdealEntityCount = 5;

	MyOctant::MyOctant(uint a_nMaxLevel, uint a_nIdealEntityCount)
	{
		// Initialize
		Init();
		m_uOctantCount = 0;
		m_uMaxLevel = a_nMaxLevel;
		m_uIdealEntityCount = a_nIdealEntityCount;
		m_uID = m_uOctantCount;

		// Set root
		m_pRoot = this;
		m_lChild.clear();

		// Min and max vector points
		std::vector<vector3> points;
		int entityCount = m_pEntityMngr->GetEntityCount();

		// Get min and max points
		for (int i = 0; i < entityCount; i++)
		{
			MyEntity* entityPointer = m_pEntityMngr->GetEntity(i);
			MyRigidBody* rigidPointer = entityPointer->GetRigidBody();

			points.push_back(rigidPointer->GetMaxGlobal());
			points.push_back(rigidPointer->GetMinGlobal());
		}

		MyRigidBody* rigidPointer = new MyRigidBody(points);

		// Get largest length
		vector3 length = rigidPointer->GetHalfWidth();
		float size = length.x;

		if (length.y > size)
		{
			size = length.y;
		}

		if (length.z > size)
		{
			size = length.z;
		}

		m_v3Center = rigidPointer->GetCenterLocal();

		// Set values, increment, construct
		m_fSize = size * 2.0f;
		m_v3Max = m_v3Center + vector3(size);
		m_v3Min = m_v3Center - vector3(size);

		m_uOctantCount += 1;

		ConstructTree(m_uMaxLevel);

		// Clean up
		SafeDelete(rigidPointer);
		points.clear();
	}

	MyOctant::MyOctant(vector3 a_v3Center, float a_fSize)
	{
		// Initialize
		Init();

		// Set values and increment
		m_v3Center = a_v3Center;
		m_fSize = a_fSize;
		m_v3Min = m_v3Center - (vector3((m_fSize / 2.0f)));
		m_v3Max = m_v3Center + (vector3((m_fSize / 2.0f)));

		m_uOctantCount++;
	}

	MyOctant::MyOctant(MyOctant const& other)
	{
		m_pParent = other.m_pParent;
		m_uID = other.m_uID;
		m_uLevel = other.m_uLevel;
		m_pRoot = other.m_pRoot;

		for (int i = 0; i < 8; i++)
		{
			m_pChild[i] = other.m_pChild[i];
		}
		m_lChild = other.m_lChild;
		m_uChildren = other.m_uChildren;

		m_fSize = other.m_fSize;
		m_v3Center = other.m_v3Center;
		m_v3Min = other.m_v3Min;
		m_v3Max = other.m_v3Max;

		m_pMeshMngr = MeshManager::GetInstance();
		m_pEntityMngr = MyEntityManager::GetInstance();
	}

	MyOctant& MyOctant::operator=(MyOctant const& other)
	{
		if (this != &other)
		{
			Release();
			Init();
			MyOctant otherOct(other);
			Swap(otherOct);
		}
		return *this;
	}

	MyOctant::~MyOctant(void)
	{
		Release();
	}

	void MyOctant::Swap(MyOctant& other)
	{
		std::swap(m_pParent, other.m_pParent);
		std::swap(m_uID, other.m_uID);
		std::swap(m_uLevel, other.m_uLevel);
		std::swap(m_pRoot, other.m_pRoot);

		for (int i = 0; i < 8; i++)
		{
			std::swap(m_pChild[i], other.m_pChild[i]);
		}
		std::swap(m_lChild, other.m_lChild);
		std::swap(m_uChildren, other.m_uChildren);

		std::swap(m_fSize, other.m_fSize);
		std::swap(m_v3Center, other.m_v3Center);
		std::swap(m_v3Min, other.m_v3Min);
		std::swap(m_v3Max, other.m_v3Max);

		m_pMeshMngr = MeshManager::GetInstance();
		m_pEntityMngr = MyEntityManager::GetInstance();
	}

	float MyOctant::GetSize(void)
	{
		return m_fSize;
	}

	vector3 MyOctant::GetCenterGlobal(void)
	{
		return m_v3Center;
	}

	vector3 MyOctant::GetMinGlobal(void)
	{
		return m_v3Min;
	}

	vector3 MyOctant::GetMaxGlobal(void)
	{
		return m_v3Max;
	}

	bool MyOctant::IsColliding(uint a_uRBIndex)
	{
		// Make sure target is in octree
		int entityCount = m_pEntityMngr->GetEntityCount();
		if (a_uRBIndex >= entityCount)
		{
			return false;
		}

		// Get min max vectors for collision checking
		MyEntity* entityPointer = m_pEntityMngr->GetEntity(a_uRBIndex);
		MyRigidBody* rigidPointer = entityPointer->GetRigidBody();
		vector3 collideMin = rigidPointer->GetMinGlobal();
		vector3 collideMax = rigidPointer->GetMaxGlobal();

		bool IsColliding = true;

		// X
		if (m_v3Max.x < collideMin.x)
		{
			IsColliding = false;
		}

		if (m_v3Min.x > collideMax.x)
		{
			IsColliding = false;
		}

		// Y
		if (m_v3Max.y < collideMin.y)
		{
			IsColliding = false;
		}

		if (m_v3Min.y > collideMax.y)
		{
			IsColliding = false;
		}

		// Z
		if (m_v3Max.z < collideMin.z)
		{
			IsColliding = false;
		}

		if (m_v3Min.z > collideMax.z)
		{
			IsColliding = false;
		}

		return IsColliding;
	}

	void MyOctant::Display(uint a_nIndex, vector3 a_v3Color)
	{
		if (m_uID == a_nIndex)
		{
			m_pMeshMngr->AddWireCubeToRenderList(
				glm::translate(IDENTITY_M4, m_v3Center) * glm::scale(vector3(m_fSize)),
				a_v3Color, RENDER_WIRE
			);
		};

		for (int i = 0; i < m_uChildren; i++)
		{
			m_pChild[i]->Display(a_nIndex, a_v3Color);
		}
	}

	void MyOctant::Display(vector3 a_v3Color)
	{
		m_pMeshMngr->AddWireCubeToRenderList(
			glm::translate(IDENTITY_M4, m_v3Center) * glm::scale(vector3(m_fSize)),
			a_v3Color, RENDER_WIRE
		);

		for (int i = 0; i < m_uChildren; i++)
		{
			m_pChild[i]->Display(a_v3Color);
		}
	}

	void MyOctant::DisplayLeafs(vector3 a_v3Color)
	{
		m_pMeshMngr->AddWireCubeToRenderList(
			glm::translate(IDENTITY_M4, m_v3Center) * glm::scale(vector3(m_fSize)),
			a_v3Color, RENDER_WIRE);

		int leaves = m_lChild.size();
		for (int i = 0; i < leaves; i++)
		{
			m_pChild[i]->DisplayLeafs(a_v3Color);
		}
	}

	void MyOctant::ClearEntityList(void)
	{
		m_EntityList.clear();

		if (!IsLeaf())
		{
			for (int i = 0; i < 8; i++)
			{
				if (m_pChild[i] != NULL)
				{
					m_pChild[i]->ClearEntityList();
				}
			}
		}
	}

	void MyOctant::Subdivide(void)
	{
		// Check if already divided or at max
		if (m_uChildren != 0)
		{
			return;
		}

		if (m_uLevel >= m_uMaxLevel)
		{
			return;
		}

		m_uChildren = 8;

		float size = m_fSize / 4.0f;
		float halfSize = size * 2.0f;

		vector3 center = m_v3Center;
		center.x -= size;
		center.y -= size;
		center.z -= size;

		// Create children
		m_pChild[0] = new MyOctant(center, halfSize);
		center.x += halfSize;
		m_pChild[1] = new MyOctant(center, halfSize);
		center.z += halfSize;
		m_pChild[2] = new MyOctant(center, halfSize);
		center.x -= halfSize;
		m_pChild[3] = new MyOctant(center, halfSize);
		center.y += halfSize;
		m_pChild[4] = new MyOctant(center, halfSize);
		center.z -= halfSize;
		m_pChild[5] = new MyOctant(center, halfSize);
		center.x += halfSize;
		m_pChild[6] = new MyOctant(center, halfSize);
		center.z += halfSize;
		m_pChild[7] = new MyOctant(center, halfSize);

		for (int i = 0; i < 8; i++)
		{
			m_pChild[i]->m_pParent = this;
			m_pChild[i]->m_pRoot = m_pRoot;
			m_pChild[i]->m_uLevel = m_uLevel + 1;
			if (m_pChild[i]->ContainsMoreThan(m_uIdealEntityCount))
			{
				m_pChild[i]->Subdivide();
			}
		}
	}

	MyOctant* MyOctant::GetChild(uint a_nChild)
	{
		if (a_nChild > 7)
		{
			return nullptr;
		}
		return m_pChild[a_nChild];
	}

	MyOctant* MyOctant::GetParent(void)
	{
		return m_pParent;
	}

	bool MyOctant::IsLeaf(void)
	{
		if (m_uChildren == 0)
		{
			return true;
		}

		return false;
	}

	bool MyOctant::ContainsMoreThan(uint a_nEntities)
	{
		int entityNum = 0;
		int entityCount = m_pEntityMngr->GetEntityCount();

		// Count colliding entities
		for (int i = 0; i < entityCount; i++)
		{
			if (IsColliding(i))
			{
				entityNum += 1;
			}

			if (entityNum > a_nEntities)
			{
				return true;
			}
		}

		return false;
	}

	void MyOctant::KillBranches(void)
	{
		for (int i = 0; i < m_uChildren; i++)
		{
			m_pChild[i]->KillBranches();
			delete m_pChild[i];
			m_pChild[i] = nullptr;
		}

		m_uChildren = 0;
	}

	void MyOctant::ConstructTree(uint a_nMaxLevel)
	{
		// Check to make sure at root
		if (m_uLevel != 0)
		{
			return;
		}

		m_uMaxLevel = a_nMaxLevel;
		m_uOctantCount = 1;

		// Clean up before construction
		m_EntityList.clear();
		KillBranches();
		m_lChild.clear();

		if (ContainsMoreThan(m_uIdealEntityCount))
		{
			Subdivide();
		}

		AssignIDtoEntity();
		ConstructList();
	}

	void MyOctant::AssignIDtoEntity(void)
	{
		if (IsLeaf())
		{
			int entityCount = m_pEntityMngr->GetEntityCount();

			for (int i = 0; i < entityCount; i++)
			{
				if (IsColliding(i))
				{
					m_EntityList.push_back(i);

					m_pEntityMngr->AddDimension(i, m_uID);
				}
			}
		}

		for (int i = 0; i < m_uChildren; i++)
		{
			m_pChild[i]->AssignIDtoEntity();
		}
	}

	uint MyOctant::GetOctantCount(void)
	{
		return m_uOctantCount;
	}

	void MyOctant::Release(void)
	{
		// Clean up
		if (m_uLevel == 0)
		{
			KillBranches();
		}
		
		m_uChildren = 0;
		m_fSize = 0;
		m_EntityList.clear();
		m_lChild.clear();
	}

	void MyOctant::Init(void)
	{
		// Manager references
		m_pEntityMngr = MyEntityManager::GetInstance();
		m_pMeshMngr = MeshManager::GetInstance();

		m_uChildren = 0;
		m_uLevel = 0;
		m_uID = m_uOctantCount;
		m_fSize = 0.0f;
		m_v3Center = vector3(0.0f, 0.0f, 0.0f);
		m_v3Max = vector3(0.0f, 0.0f, 0.0f);
		m_v3Min = vector3(0.0f, 0.0f, 0.0f);

		// Null pointers
		m_pRoot = nullptr;
		m_pParent = nullptr;
		for (int i = 0; i < 8; i++)
		{
			m_pChild[i] = nullptr;
		}
	}

	void MyOctant::ConstructList(void)
	{
		if (m_EntityList.size() > 0)
		{
			m_pRoot->m_lChild.push_back(this);
		}

		for (int i = 0; i < m_uChildren; i++)
		{
			m_pChild[i]->ConstructList();
		}
	}
}