#pragma once
#pragma warning(suppress : 26495)
#include <cmath>
#include <cstdint>

#ifdef _WIN32
#define __rescalll __thiscall
#else
#define __rescalll __attribute__((__cdecl__))
#endif

struct Vector {
    float x, y, z;
    inline float Length() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }
    inline float Length2D() const
    {
        return std::sqrt(x * x + y * y);
    }
    inline float Dot(const Vector& vOther) const
    {
        return Vector::DotProduct(*this, vOther);
    }
    inline Vector operator*(float fl)
    {
        Vector res;
        res.x = x * fl;
        res.y = y * fl;
        res.z = z * fl;
        return res;
    }
    inline Vector operator+(Vector vec)
    {
        Vector res;
        res.x = x + vec.x;
        res.y = y + vec.y;
        res.z = z + vec.z;
        return res;
    }
    inline float& operator[](int i)
    {
        return ((float*)this)[i];
    }
    inline float operator[](int i) const
    {
        return ((float*)this)[i];
    }
    static inline float DotProduct(const Vector& a, const Vector& b) 
    {
        return a.x*b.x + a.y*b.y + a.z*b.z; 
    }
};

struct QAngle {
    float x, y, z;
};

struct Color {
    Color()
    {
        *((int*)this) = 255;
    }
    Color(int _r, int _g, int _b)
    {
        SetColor(_r, _g, _b, 255);
    }
    Color(int _r, int _g, int _b, int _a)
    {
        SetColor(_r, _g, _b, _a);
    }
    void SetColor(int _r, int _g, int _b, int _a = 255)
    {
        _color[0] = (unsigned char)_r;
        _color[1] = (unsigned char)_g;
        _color[2] = (unsigned char)_b;
        _color[3] = (unsigned char)_a;
    }
    inline int r() const { return _color[0]; }
    inline int g() const { return _color[1]; }
    inline int b() const { return _color[2]; }
    inline int a() const { return _color[3]; }
    unsigned char _color[4] = { 0, 0, 0, 0 };
};

#pragma region CUTL_VEC
#define Q_ARRAYSIZE(p) (sizeof(p) / sizeof(p[0]))

#define Assert(a) ((void)0)
#define UTLMEMORY_TRACK_ALLOC() ((void)0)
#define UTLMEMORY_TRACK_FREE() ((void)0)
#define MEM_ALLOC_CREDIT_CLASS() ((void)0)

template <class T>
inline void Construct(T* pMemory)
{
    ::new (pMemory) T;
}
template <class T>
inline void CopyConstruct(T* pMemory, T const& src)
{
    ::new (pMemory) T(src);
}
template <class T>
inline void Destruct(T* pMemory)
{
    pMemory->~T();
}

template <class T, class I = int>
class CUtlMemory {
public:
    CUtlMemory(int nGrowSize = 0, int nInitSize = 0);
    CUtlMemory(T* pMemory, int numElements);
    CUtlMemory(const T* pMemory, int numElements);
    ~CUtlMemory();
    void Init(int nGrowSize = 0, int nInitSize = 0);

    class Iterator_t {
    public:
        Iterator_t(I i)
            : index(i)
        {
        }
        I index;

        bool operator==(const Iterator_t it) const { return index == it.index; }
        bool operator!=(const Iterator_t it) const { return index != it.index; }
    };

    Iterator_t First() const { return Iterator_t(IsIdxValid(0) ? 0 : InvalidIndex()); }
    Iterator_t Next(const Iterator_t& it) const { return Iterator_t(IsIdxValid(it.index + 1) ? it.index + 1 : InvalidIndex()); }
    I GetIndex(const Iterator_t& it) const { return it.index; }
    bool IsIdxAfter(I i, const Iterator_t& it) const { return i > it.index; }
    bool IsValidIterator(const Iterator_t& it) const { return IsIdxValid(it.index); }
    Iterator_t InvalidIterator() const { return Iterator_t(InvalidIndex()); }
    T& operator[](I i);
    const T& operator[](I i) const;
    T& Element(I i);
    const T& Element(I i) const;
    bool IsIdxValid(I i) const;
    static I InvalidIndex() { return (I)-1; }
    T* Base();
    const T* Base() const;
    void SetExternalBuffer(T* pMemory, int numElements);
    void SetExternalBuffer(const T* pMemory, int numElements);
    void AssumeMemory(T* pMemory, int nSize);
    void Swap(CUtlMemory<T, I>& mem);
    void ConvertToGrowableMemory(int nGrowSize);
    int NumAllocated() const;
    int Count() const;
    void Grow(int num = 1);
    void EnsureCapacity(int num);
    void Purge();
    void Purge(int numElements);
    bool IsExternallyAllocated() const;
    bool IsReadOnly() const;
    void SetGrowSize(int size);

public:
    void ValidateGrowSize()
    {
    }

    enum {
        EXTERNAL_BUFFER_MARKER = -1,
        EXTERNAL_CONST_BUFFER_MARKER = -2,
    };

    T* m_pMemory;
    int m_nAllocationCount;
    int m_nGrowSize;
};

template <class T, class A = CUtlMemory<T>>
class CUtlVector {
    typedef A CAllocator;

public:
    typedef T ElemType_t;

    CUtlVector(int growSize = 0, int initSize = 0);
    CUtlVector(T* pMemory, int allocationCount, int numElements = 0);
    ~CUtlVector();
    CUtlVector<T, A>& operator=(const CUtlVector<T, A>& other);
    T& operator[](int i);
    const T& operator[](int i) const;
    T& Element(int i);
    const T& Element(int i) const;
    T& Head();
    const T& Head() const;
    T& Tail();
    const T& Tail() const;
    T* Base() { return m_Memory.Base(); }
    const T* Base() const { return m_Memory.Base(); }
    int Count() const;
    int Size() const;
    bool IsValidIndex(int i) const;
    static int InvalidIndex();
    int AddToHead();
    int AddToTail();
    int InsertBefore(int elem);
    int InsertAfter(int elem);
    int AddToHead(const T& src);
    int AddToTail(const T& src);
    int InsertBefore(int elem, const T& src);
    int InsertAfter(int elem, const T& src);
    int AddMultipleToHead(int num);
    int AddMultipleToTail(int num, const T* pToCopy = NULL);
    int InsertMultipleBefore(int elem, int num, const T* pToCopy = NULL);
    int InsertMultipleAfter(int elem, int num);
    void SetSize(int size);
    void SetCount(int count);
    void CopyArray(const T* pArray, int size);
    void Swap(CUtlVector<T, A>& vec);
    int AddVectorToTail(CUtlVector<T, A> const& src);
    int Find(const T& src) const;
    bool HasElement(const T& src) const;
    void EnsureCapacity(int num);
    void EnsureCount(int num);
    void FastRemove(int elem);
    void Remove(int elem);
    bool FindAndRemove(const T& src);
    void RemoveMultiple(int elem, int num);
    void RemoveAll();
    void Purge();
    void PurgeAndDeleteElements();
    void Compact();
    void SetGrowSize(int size) { m_Memory.SetGrowSize(size); }
    int NumAllocated() const;
    void Sort(int(__cdecl* pfnCompare)(const T*, const T*));

public:
    //CUtlVector(CUtlVector const& vec) { Assert(0); }
    void GrowVector(int num = 1);
    void ShiftElementsRight(int elem, int num = 1);
    void ShiftElementsLeft(int elem, int num = 1);

    CAllocator m_Memory;
    int m_Size;
    T* m_pElements;

    inline void ResetDbgInfo() { m_pElements = Base(); }
};

template <typename T, class A>
inline CUtlVector<T, A>::CUtlVector(int growSize, int initSize)
    : m_Memory(growSize, initSize)
    , m_Size(0)
{
    ResetDbgInfo();
}
template <typename T, class A>
inline CUtlVector<T, A>::CUtlVector(T* pMemory, int allocationCount, int numElements)
    : m_Memory(pMemory, allocationCount)
    , m_Size(numElements)
{
    ResetDbgInfo();
}
template <typename T, class A>
inline CUtlVector<T, A>::~CUtlVector()
{
    Purge();
}
template <typename T, class A>
inline CUtlVector<T, A>& CUtlVector<T, A>::operator=(const CUtlVector<T, A>& other)
{
    int nCount = other.Count();
    SetSize(nCount);
    for (int i = 0; i < nCount; i++) {
        (*this)[i] = other[i];
    }
    return *this;
}
template <typename T, class A>
inline T& CUtlVector<T, A>::operator[](int i)
{
    return m_Memory[i];
}
template <typename T, class A>
inline const T& CUtlVector<T, A>::operator[](int i) const
{
    return m_Memory[i];
}
template <typename T, class A>
inline T& CUtlVector<T, A>::Element(int i)
{
    return m_Memory[i];
}
template <typename T, class A>
inline const T& CUtlVector<T, A>::Element(int i) const
{
    return m_Memory[i];
}
template <typename T, class A>
inline T& CUtlVector<T, A>::Head()
{
    Assert(m_Size > 0);
    return m_Memory[0];
}
template <typename T, class A>
inline const T& CUtlVector<T, A>::Head() const
{
    Assert(m_Size > 0);
    return m_Memory[0];
}
template <typename T, class A>
inline T& CUtlVector<T, A>::Tail()
{
    Assert(m_Size > 0);
    return m_Memory[m_Size - 1];
}
template <typename T, class A>
inline const T& CUtlVector<T, A>::Tail() const
{
    Assert(m_Size > 0);
    return m_Memory[m_Size - 1];
}
template <typename T, class A>
inline int CUtlVector<T, A>::Size() const
{
    return m_Size;
}
template <typename T, class A>
inline int CUtlVector<T, A>::Count() const
{
    return m_Size;
}
template <typename T, class A>
inline bool CUtlVector<T, A>::IsValidIndex(int i) const
{
    return (i >= 0) && (i < m_Size);
}
template <typename T, class A>
inline int CUtlVector<T, A>::InvalidIndex()
{
    return -1;
}
template <typename T, class A>
void CUtlVector<T, A>::GrowVector(int num)
{
    if (m_Size + num > m_Memory.NumAllocated()) {
        MEM_ALLOC_CREDIT_CLASS();
        m_Memory.Grow(m_Size + num - m_Memory.NumAllocated());
    }

    m_Size += num;
    ResetDbgInfo();
}
template <typename T, class A>
void CUtlVector<T, A>::Sort(int(__cdecl* pfnCompare)(const T*, const T*))
{
    typedef int(__cdecl * QSortCompareFunc_t)(const void*, const void*);
    if (Count() <= 1)
        return;

    if (Base()) {
        qsort(Base(), Count(), sizeof(T), (QSortCompareFunc_t)(pfnCompare));
    } else {
        Assert(0);

        for (int i = m_Size - 1; i >= 0; --i) {
            for (int j = 1; j <= i; ++j) {
                if (pfnCompare(&Element(j - 1), &Element(j)) < 0) {
                    swap(Element(j - 1), Element(j));
                }
            }
        }
    }
}
template <typename T, class A>
void CUtlVector<T, A>::EnsureCapacity(int num)
{
    MEM_ALLOC_CREDIT_CLASS();
    m_Memory.EnsureCapacity(num);
    ResetDbgInfo();
}
template <typename T, class A>
void CUtlVector<T, A>::EnsureCount(int num)
{
    if (Count() < num)
        AddMultipleToTail(num - Count());
}
template <typename T, class A>
void CUtlVector<T, A>::ShiftElementsRight(int elem, int num)
{
    Assert(IsValidIndex(elem) || (m_Size == 0) || (num == 0));
    int numToMove = m_Size - elem - num;
    if ((numToMove > 0) && (num > 0))
        memmove(&Element(elem + num), &Element(elem), numToMove * sizeof(T));
}
template <typename T, class A>
void CUtlVector<T, A>::ShiftElementsLeft(int elem, int num)
{
    Assert(IsValidIndex(elem) || (m_Size == 0) || (num == 0));
    int numToMove = m_Size - elem - num;
    if ((numToMove > 0) && (num > 0)) {
        memmove(&Element(elem), &Element(elem + num), numToMove * sizeof(T));
    }
}
template <typename T, class A>
inline int CUtlVector<T, A>::AddToHead()
{
    return InsertBefore(0);
}
template <typename T, class A>
inline int CUtlVector<T, A>::AddToTail()
{
    return InsertBefore(m_Size);
}
template <typename T, class A>
inline int CUtlVector<T, A>::InsertAfter(int elem)
{
    return InsertBefore(elem + 1);
}
template <typename T, class A>
int CUtlVector<T, A>::InsertBefore(int elem)
{
    Assert((elem == Count()) || IsValidIndex(elem));

    GrowVector();
    ShiftElementsRight(elem);
    Construct(&Element(elem));
    return elem;
}
template <typename T, class A>
inline int CUtlVector<T, A>::AddToHead(const T& src)
{
    Assert((Base() == NULL) || (&src < Base()) || (&src >= (Base() + Count())));
    return InsertBefore(0, src);
}
template <typename T, class A>
inline int CUtlVector<T, A>::AddToTail(const T& src)
{
    Assert((Base() == NULL) || (&src < Base()) || (&src >= (Base() + Count())));
    return InsertBefore(m_Size, src);
}
template <typename T, class A>
inline int CUtlVector<T, A>::InsertAfter(int elem, const T& src)
{
    Assert((Base() == NULL) || (&src < Base()) || (&src >= (Base() + Count())));
    return InsertBefore(elem + 1, src);
}
template <typename T, class A>
int CUtlVector<T, A>::InsertBefore(int elem, const T& src)
{
    Assert((Base() == NULL) || (&src < Base()) || (&src >= (Base() + Count())));

    Assert((elem == Count()) || IsValidIndex(elem));

    GrowVector();
    ShiftElementsRight(elem);
    CopyConstruct(&Element(elem), src);
    return elem;
}
template <typename T, class A>
inline int CUtlVector<T, A>::AddMultipleToHead(int num)
{
    return InsertMultipleBefore(0, num);
}
template <typename T, class A>
inline int CUtlVector<T, A>::AddMultipleToTail(int num, const T* pToCopy)
{
    Assert((Base() == NULL) || !pToCopy || (pToCopy + num < Base()) || (pToCopy >= (Base() + Count())));

    return InsertMultipleBefore(m_Size, num, pToCopy);
}
template <typename T, class A>
int CUtlVector<T, A>::InsertMultipleAfter(int elem, int num)
{
    return InsertMultipleBefore(elem + 1, num);
}
template <typename T, class A>
void CUtlVector<T, A>::SetCount(int count)
{
    RemoveAll();
    AddMultipleToTail(count);
}
template <typename T, class A>
inline void CUtlVector<T, A>::SetSize(int size)
{
    SetCount(size);
}
template <typename T, class A>
void CUtlVector<T, A>::CopyArray(const T* pArray, int size)
{
    Assert((Base() == NULL) || !pArray || (Base() >= (pArray + size)) || (pArray >= (Base() + Count())));

    SetSize(size);
    for (int i = 0; i < size; i++) {
        (*this)[i] = pArray[i];
    }
}
template <typename T, class A>
void CUtlVector<T, A>::Swap(CUtlVector<T, A>& vec)
{
    m_Memory.Swap(vec.m_Memory);
    swap(m_Size, vec.m_Size);
    swap(m_pElements, vec.m_pElements);
}
template <typename T, class A>
int CUtlVector<T, A>::AddVectorToTail(CUtlVector const& src)
{
    Assert(&src != this);

    int base = Count();

    AddMultipleToTail(src.Count());

    for (int i = 0; i < src.Count(); i++) {
        (*this)[base + i] = src[i];
    }

    return base;
}
template <typename T, class A>
inline int CUtlVector<T, A>::InsertMultipleBefore(int elem, int num, const T* pToInsert)
{
    if (num == 0)
        return elem;

    Assert((elem == Count()) || IsValidIndex(elem));

    GrowVector(num);
    ShiftElementsRight(elem, num);

    for (int i = 0; i < num; ++i)
        Construct(&Element(elem + i));

    if (pToInsert) {
        for (int i = 0; i < num; i++) {
            Element(elem + i) = pToInsert[i];
        }
    }

    return elem;
}
template <typename T, class A>
int CUtlVector<T, A>::Find(const T& src) const
{
    for (int i = 0; i < Count(); ++i) {
        if (Element(i) == src)
            return i;
    }
    return -1;
}
template <typename T, class A>
bool CUtlVector<T, A>::HasElement(const T& src) const
{
    return (Find(src) >= 0);
}
template <typename T, class A>
void CUtlVector<T, A>::FastRemove(int elem)
{
    Assert(IsValidIndex(elem));

    Destruct(&Element(elem));
    if (m_Size > 0) {
        memcpy(&Element(elem), &Element(m_Size - 1), sizeof(T));
        --m_Size;
    }
}
template <typename T, class A>
void CUtlVector<T, A>::Remove(int elem)
{
    Destruct(&Element(elem));
    ShiftElementsLeft(elem);
    --m_Size;
}
template <typename T, class A>
bool CUtlVector<T, A>::FindAndRemove(const T& src)
{
    int elem = Find(src);
    if (elem != -1) {
        Remove(elem);
        return true;
    }
    return false;
}
template <typename T, class A>
void CUtlVector<T, A>::RemoveMultiple(int elem, int num)
{
    Assert(elem >= 0);
    Assert(elem + num <= Count());

    for (int i = elem + num; --i >= elem;)
        Destruct(&Element(i));

    ShiftElementsLeft(elem, num);
    m_Size -= num;
}
template <typename T, class A>
void CUtlVector<T, A>::RemoveAll()
{
    for (int i = m_Size; --i >= 0;) {
        Destruct(&Element(i));
    }

    m_Size = 0;
}
template <typename T, class A>
inline void CUtlVector<T, A>::Purge()
{
    RemoveAll();
    m_Memory.Purge();
    ResetDbgInfo();
}
template <typename T, class A>
inline void CUtlVector<T, A>::PurgeAndDeleteElements()
{
    for (int i = 0; i < m_Size; i++) {
        delete Element(i);
    }
    Purge();
}
template <typename T, class A>
inline void CUtlVector<T, A>::Compact()
{
    m_Memory.Purge(m_Size);
}
template <typename T, class A>
inline int CUtlVector<T, A>::NumAllocated() const
{
    return m_Memory.NumAllocated();
}

template <class T, class I>
CUtlMemory<T, I>::CUtlMemory(int nGrowSize, int nInitAllocationCount)
    : m_pMemory(0)
    , m_nAllocationCount(nInitAllocationCount)
    , m_nGrowSize(nGrowSize)
{
    ValidateGrowSize();
    Assert(nGrowSize >= 0);
    if (m_nAllocationCount) {
        UTLMEMORY_TRACK_ALLOC();
        MEM_ALLOC_CREDIT_CLASS();
        m_pMemory = (T*)malloc(m_nAllocationCount * sizeof(T));
    }
}
template <class T, class I>
CUtlMemory<T, I>::CUtlMemory(T* pMemory, int numElements)
    : m_pMemory(pMemory)
    , m_nAllocationCount(numElements)
{
    m_nGrowSize = EXTERNAL_BUFFER_MARKER;
}
template <class T, class I>
CUtlMemory<T, I>::CUtlMemory(const T* pMemory, int numElements)
    : m_pMemory((T*)pMemory)
    , m_nAllocationCount(numElements)
{
    m_nGrowSize = EXTERNAL_CONST_BUFFER_MARKER;
}
template <class T, class I>
CUtlMemory<T, I>::~CUtlMemory()
{
    Purge();
}
template <class T, class I>
void CUtlMemory<T, I>::Init(int nGrowSize /*= 0*/, int nInitSize /*= 0*/)
{
    Purge();

    m_nGrowSize = nGrowSize;
    m_nAllocationCount = nInitSize;
    ValidateGrowSize();
    Assert(nGrowSize >= 0);
    if (m_nAllocationCount) {
        UTLMEMORY_TRACK_ALLOC();
        MEM_ALLOC_CREDIT_CLASS();
        m_pMemory = (T*)malloc(m_nAllocationCount * sizeof(T));
    }
}
template <class T, class I>
void CUtlMemory<T, I>::Swap(CUtlMemory<T, I>& mem)
{
    swap(m_nGrowSize, mem.m_nGrowSize);
    swap(m_pMemory, mem.m_pMemory);
    swap(m_nAllocationCount, mem.m_nAllocationCount);
}
template <class T, class I>
void CUtlMemory<T, I>::ConvertToGrowableMemory(int nGrowSize)
{
    if (!IsExternallyAllocated())
        return;

    m_nGrowSize = nGrowSize;
    if (m_nAllocationCount) {
        UTLMEMORY_TRACK_ALLOC();
        MEM_ALLOC_CREDIT_CLASS();

        int nNumBytes = m_nAllocationCount * sizeof(T);
        T* pMemory = (T*)malloc(nNumBytes);
        memcpy(pMemory, m_pMemory, nNumBytes);
        m_pMemory = pMemory;
    } else {
        m_pMemory = NULL;
    }
}
template <class T, class I>
void CUtlMemory<T, I>::SetExternalBuffer(T* pMemory, int numElements)
{
    Purge();

    m_pMemory = pMemory;
    m_nAllocationCount = numElements;

    m_nGrowSize = EXTERNAL_BUFFER_MARKER;
}

template <class T, class I>
void CUtlMemory<T, I>::SetExternalBuffer(const T* pMemory, int numElements)
{
    Purge();

    m_pMemory = const_cast<T*>(pMemory);
    m_nAllocationCount = numElements;

    m_nGrowSize = EXTERNAL_CONST_BUFFER_MARKER;
}
template <class T, class I>
void CUtlMemory<T, I>::AssumeMemory(T* pMemory, int numElements)
{
    Purge();

    m_pMemory = pMemory;
    m_nAllocationCount = numElements;
}
template <class T, class I>
inline T& CUtlMemory<T, I>::operator[](I i)
{
    Assert(!IsReadOnly());
    Assert(IsIdxValid(i));
    return m_pMemory[i];
}
template <class T, class I>
inline const T& CUtlMemory<T, I>::operator[](I i) const
{
    Assert(IsIdxValid(i));
    return m_pMemory[i];
}
template <class T, class I>
inline T& CUtlMemory<T, I>::Element(I i)
{
    Assert(!IsReadOnly());
    Assert(IsIdxValid(i));
    return m_pMemory[i];
}
template <class T, class I>
inline const T& CUtlMemory<T, I>::Element(I i) const
{
    Assert(IsIdxValid(i));
    return m_pMemory[i];
}
template <class T, class I>
bool CUtlMemory<T, I>::IsExternallyAllocated() const
{
    return (m_nGrowSize < 0);
}
template <class T, class I>
bool CUtlMemory<T, I>::IsReadOnly() const
{
    return (m_nGrowSize == EXTERNAL_CONST_BUFFER_MARKER);
}
template <class T, class I>
void CUtlMemory<T, I>::SetGrowSize(int nSize)
{
    Assert(!IsExternallyAllocated());
    Assert(nSize >= 0);
    m_nGrowSize = nSize;
    ValidateGrowSize();
}
template <class T, class I>
inline T* CUtlMemory<T, I>::Base()
{
    Assert(!IsReadOnly());
    return m_pMemory;
}
template <class T, class I>
inline const T* CUtlMemory<T, I>::Base() const
{
    return m_pMemory;
}
template <class T, class I>
inline int CUtlMemory<T, I>::NumAllocated() const
{
    return m_nAllocationCount;
}
template <class T, class I>
inline int CUtlMemory<T, I>::Count() const
{
    return m_nAllocationCount;
}
template <class T, class I>
inline bool CUtlMemory<T, I>::IsIdxValid(I i) const
{
    return (((int)i) >= 0) && (((int)i) < m_nAllocationCount);
}
inline int UtlMemory_CalcNewAllocationCount(int nAllocationCount, int nGrowSize, int nNewSize, int nBytesItem)
{
    if (nGrowSize) {
        nAllocationCount = ((1 + ((nNewSize - 1) / nGrowSize)) * nGrowSize);
    } else {
        if (!nAllocationCount) {
            nAllocationCount = (31 + nBytesItem) / nBytesItem;
        }

        while (nAllocationCount < nNewSize) {
            nAllocationCount *= 2;
        }
    }

    return nAllocationCount;
}
template <class T, class I>
void CUtlMemory<T, I>::Grow(int num)
{
    Assert(num > 0);

    if (IsExternallyAllocated()) {
        Assert(0);
        return;
    }

    int nAllocationRequested = m_nAllocationCount + num;

    UTLMEMORY_TRACK_FREE();

    m_nAllocationCount = UtlMemory_CalcNewAllocationCount(m_nAllocationCount, m_nGrowSize, nAllocationRequested, sizeof(T));

    if ((int)(I)m_nAllocationCount < nAllocationRequested) {
        if ((int)(I)m_nAllocationCount == 0 && (int)(I)(m_nAllocationCount - 1) >= nAllocationRequested) {
            --m_nAllocationCount;
        } else {
            if ((int)(I)nAllocationRequested != nAllocationRequested) {
                Assert(0);
                return;
            }
            while ((int)(I)m_nAllocationCount < nAllocationRequested) {
                m_nAllocationCount = (m_nAllocationCount + nAllocationRequested) / 2;
            }
        }
    }

    UTLMEMORY_TRACK_ALLOC();

    if (m_pMemory) {
        MEM_ALLOC_CREDIT_CLASS();
        m_pMemory = (T*)realloc(m_pMemory, m_nAllocationCount * sizeof(T));
        Assert(m_pMemory);
    } else {
        MEM_ALLOC_CREDIT_CLASS();
        m_pMemory = (T*)malloc(m_nAllocationCount * sizeof(T));
        Assert(m_pMemory);
    }
}
template <class T, class I>
inline void CUtlMemory<T, I>::EnsureCapacity(int num)
{
    if (m_nAllocationCount >= num)
        return;

    if (IsExternallyAllocated()) {
        Assert(0);
        return;
    }

    UTLMEMORY_TRACK_FREE();

    m_nAllocationCount = num;

    UTLMEMORY_TRACK_ALLOC();

    if (m_pMemory) {
        MEM_ALLOC_CREDIT_CLASS();
        m_pMemory = (T*)realloc(m_pMemory, m_nAllocationCount * sizeof(T));
    } else {
        MEM_ALLOC_CREDIT_CLASS();
        m_pMemory = (T*)malloc(m_nAllocationCount * sizeof(T));
    }
}
template <class T, class I>
void CUtlMemory<T, I>::Purge()
{
    if (!IsExternallyAllocated()) {
        if (m_pMemory) {
            UTLMEMORY_TRACK_FREE();
            free((void*)m_pMemory);
            m_pMemory = 0;
        }
        m_nAllocationCount = 0;
    }
}
template <class T, class I>
void CUtlMemory<T, I>::Purge(int numElements)
{
    Assert(numElements >= 0);

    if (numElements > m_nAllocationCount) {
        Assert(numElements <= m_nAllocationCount);
        return;
    }

    if (numElements == 0) {
        Purge();
        return;
    }

    if (IsExternallyAllocated()) {
        return;
    }

    if (numElements == m_nAllocationCount) {
        return;
    }

    if (!m_pMemory) {
        Assert(m_pMemory);
        return;
    }

    UTLMEMORY_TRACK_FREE();

    m_nAllocationCount = numElements;

    UTLMEMORY_TRACK_ALLOC();

    MEM_ALLOC_CREDIT_CLASS();
    m_pMemory = (T*)realloc(m_pMemory, m_nAllocationCount * sizeof(T));
}
#pragma endregion

#define FCVAR_DEVELOPMENTONLY (1 << 1)
#define FCVAR_HIDDEN (1 << 4)
#define FCVAR_NEVER_AS_STRING (1 << 12)
#define FCVAR_CHEAT (1 << 14)

#define COMMAND_COMPLETION_MAXITEMS 64
#define COMMAND_COMPLETION_ITEM_LENGTH 64

struct CCommand;
struct ConCommandBase;

typedef void (*FnChangeCallback_t)(void* var, const char* pOldValue, float flOldValue);

using _CommandCallback = void (*)(const CCommand& args);
using _CommandCompletionCallback = int (*)(const char* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);
using _InternalSetValue = void(__rescalll*)(void* thisptr, const char* value);
using _InternalSetFloatValue = void(__rescalll*)(void* thisptr, float value);
using _InternalSetIntValue = void(__rescalll*)(void* thisptr, int value);
using _RegisterConCommand = void(__rescalll*)(void* thisptr, ConCommandBase* pCommandBase);
using _UnregisterConCommand = void(__rescalll*)(void* thisptr, ConCommandBase* pCommandBase);
using _FindCommandBase = void*(__rescalll*)(void* thisptr, const char* name);
using _InstallGlobalChangeCallback = void(__rescalll*)(void* thisptr, FnChangeCallback_t callback);
using _RemoveGlobalChangeCallback = void(__rescalll*)(void* thisptr, FnChangeCallback_t callback);
using _AutoCompletionFunc = int(__rescalll*)(void* thisptr, char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

class IConVar {
public:
    virtual void SetValue(const char* pValue) = 0;
    virtual void SetValue(float flValue) = 0;
    virtual void SetValue(int nValue) = 0;
    virtual void SetValue(Color value) = 0;
    virtual const char* GetName(void) const = 0;
    virtual const char* GetBaseName(void) const = 0;
    virtual bool IsFlagSet(int nFlag) const = 0;
    virtual int GetSplitScreenPlayerSlot() const = 0;
};

struct ConCommandBase {
    void* ConCommandBase_VTable; // 0
    ConCommandBase* m_pNext; // 4
    bool m_bRegistered; // 8
    const char* m_pszName; // 12
    const char* m_pszHelpString; // 16
    int m_nFlags; // 20

    ConCommandBase(const char* name, int flags, const char* helpstr)
        : ConCommandBase_VTable(nullptr)
        , m_pNext(nullptr)
        , m_bRegistered(false)
        , m_pszName(name)
        , m_pszHelpString(helpstr)
        , m_nFlags(flags)
    {
    }
};

struct CCommand {
    enum {
        COMMAND_MAX_ARGC = 64,
        COMMAND_MAX_LENGTH = 512
    };
    int m_nArgc;
    int m_nArgv0Size;
    char m_pArgSBuffer[COMMAND_MAX_LENGTH];
    char m_pArgvBuffer[COMMAND_MAX_LENGTH];
    const char* m_ppArgv[COMMAND_MAX_ARGC];

    int ArgC() const
    {
        return this->m_nArgc;
    }
    const char* Arg(int nIndex) const
    {
        return this->m_ppArgv[nIndex];
    }
    const char* operator[](int nIndex) const
    {
        return Arg(nIndex);
    }
};

struct ConCommand : ConCommandBase {
    union {
        void* m_fnCommandCallbackV1;
        _CommandCallback m_fnCommandCallback;
        void* m_pCommandCallback;
    };

    union {
        _CommandCompletionCallback m_fnCompletionCallback;
        void* m_pCommandCompletionCallback;
    };

    bool m_bHasCompletionCallback : 1;
    bool m_bUsingNewCommandCallback : 1;
    bool m_bUsingCommandCallbackInterface : 1;

    ConCommand(const char* pName, _CommandCallback callback, const char* pHelpString, int flags, _CommandCompletionCallback completionFunc)
        : ConCommandBase(pName, flags, pHelpString)
        , m_fnCommandCallback(callback)
        , m_fnCompletionCallback(completionFunc)
        , m_bHasCompletionCallback(completionFunc != nullptr)
        , m_bUsingNewCommandCallback(true)
        , m_bUsingCommandCallbackInterface(false)
    {
    }
};

struct ConVar : ConCommandBase {
    void* ConVar_VTable; // 24
    ConVar* m_pParent; // 28
    const char* m_pszDefaultValue; // 32
    char* m_pszString; // 36
    int m_StringLength; // 40
    float m_fValue; // 44
    int m_nValue; // 48
    bool m_bHasMin; // 52
    float m_fMinVal; // 56
    bool m_bHasMax; // 60
    float m_fMaxVal; // 64
    FnChangeCallback_t m_fnChangeCallback; // 68

    ConVar(const char* name, const char* value, int flags, const char* helpstr, bool hasmin, float min, bool hasmax, float max)
        : ConCommandBase(name, flags, helpstr)
        , ConVar_VTable(nullptr)
        , m_pParent(nullptr)
        , m_pszDefaultValue(value)
        , m_pszString(nullptr)
        , m_StringLength(0)
        , m_fValue(0.0f)
        , m_nValue(0)
        , m_bHasMin(hasmin)
        , m_fMinVal(min)
        , m_bHasMax(hasmax)
        , m_fMaxVal(max)
        , m_fnChangeCallback(nullptr)
    {
    }
};

struct ConVar2 : ConCommandBase {
    void* ConVar_VTable; // 24
    ConVar2* m_pParent; // 28
    const char* m_pszDefaultValue; // 32
    char* m_pszString; // 36
    int m_StringLength; // 40
    float m_fValue; // 44
    int m_nValue; // 48
    bool m_bHasMin; // 52
    float m_fMinVal; // 56
    bool m_bHasMax; // 60
    float m_fMaxVal; // 64
    CUtlVector<FnChangeCallback_t> m_fnChangeCallback; // 68

    ConVar2(const char* name, const char* value, int flags, const char* helpstr, bool hasmin, float min, bool hasmax, float max)
        : ConCommandBase(name, flags, helpstr)
        , ConVar_VTable(nullptr)
        , m_pParent(nullptr)
        , m_pszDefaultValue(value)
        , m_pszString(nullptr)
        , m_StringLength(0)
        , m_fValue(0.0f)
        , m_nValue(0)
        , m_bHasMin(hasmin)
        , m_fMinVal(min)
        , m_bHasMax(hasmax)
        , m_fMaxVal(max)
        , m_fnChangeCallback()
    {
    }
};

#define SIGNONSTATE_NONE 0
#define SIGNONSTATE_CHALLENGE 1
#define SIGNONSTATE_CONNECTED 2
#define SIGNONSTATE_NEW 3
#define SIGNONSTATE_PRESPAWN 4
#define SIGNONSTATE_SPAWN 5
#define SIGNONSTATE_FULL 6
#define SIGNONSTATE_CHANGELEVEL 7

struct CUserCmd {
    void* VMT; // 0
    int command_number; // 4
    int tick_count; // 8
    QAngle viewangles; // 12, 16, 20
    float forwardmove; // 24
    float sidemove; // 28
    float upmove; // 32
    int buttons; // 36
    unsigned char impulse; // 40
    int weaponselect; // 44
    int weaponsubtype; // 48
    int random_seed; // 52
    short mousedx; // 56
    short mousedy; // 58
    bool hasbeenpredicted; // 60
};

#define GAMEMOVEMENT_JUMP_HEIGHT 21.0f

struct CMoveData {
    bool m_bFirstRunOfFunctions : 1; // 0
    bool m_bGameCodeMovedPlayer : 1; // 2
    void* m_nPlayerHandle; // 4
    int m_nImpulseCommand; // 8
    QAngle m_vecViewAngles; // 12, 16, 20
    QAngle m_vecAbsViewAngles; // 24, 28, 32
    int m_nButtons; // 36
    int m_nOldButtons; // 40
    float m_flForwardMove; // 44
    float m_flSideMove; // 48
    float m_flUpMove; // 52
    float m_flMaxSpeed; // 56
    float m_flClientMaxSpeed; // 60
    Vector m_vecVelocity; // 64, 68, 72
    QAngle m_vecAngles; // 76, 80, 84
    QAngle m_vecOldAngles; // 88, 92, 96
    float m_outStepHeight; // 100
    Vector m_outWishVel; // 104, 108, 112
    Vector m_outJumpVel; // 116, 120, 124
    Vector m_vecConstraintCenter; // 128, 132, 136
    float m_flConstraintRadius; // 140
    float m_flConstraintWidth; // 144
    float m_flConstraintSpeedFactor; // 148
    Vector m_vecAbsOrigin; // 152
};

class CHLMoveData : public CMoveData {
public:
    bool m_bIsSprinting;
};

#define IN_ATTACK (1 << 0)
#define IN_JUMP (1 << 1)
#define IN_DUCK (1 << 2)
#define IN_FORWARD (1 << 3)
#define IN_BACK (1 << 4)
#define IN_USE (1 << 5)
#define IN_MOVELEFT (1 << 9)
#define IN_MOVERIGHT (1 << 10)
#define IN_ATTACK2 (1 << 11)
#define IN_RELOAD (1 << 13)
#define IN_SPEED (1 << 17)

#define FL_ONGROUND (1 << 0)
#define FL_DUCKING (1 << 1)
#define FL_FROZEN (1 << 5)
#define FL_ATCONTROLS (1 << 6)

#define WL_Feet 1
#define WL_Waist 2

#define MOVETYPE_NOCLIP 8
#define MOVETYPE_LADDER 9

typedef enum {
    HS_NEW_GAME = 0,
    HS_LOAD_GAME = 1,
    HS_CHANGE_LEVEL_SP = 2,
    HS_CHANGE_LEVEL_MP = 3,
    HS_RUN = 4,
    HS_GAME_SHUTDOWN = 5,
    HS_SHUTDOWN = 6,
    HS_RESTART = 7,

    INFRA_HS_NEW_GAME = 0,
    INFRA_HS_LOAD_GAME = 1,
    INFRA_HS_LOAD_GAME_WITHOUT_RESTART = 2,
    INFRA_HS_CHANGE_LEVEL_SP = 3,
    INFRA_HS_CHANGE_LEVEL_MP = 4,
    INFRA_HS_RUN = 5,
    INFRA_HS_GAME_SHUTDOWN = 6,
    INFRA_HS_SHUTDOWN = 7,
    INFRA_HS_RESTART = 8,
    INFRA_HS_RESTART_WITHOUT_RESTART = 9
} HOSTSTATES;

struct CHostState {
    int m_currentState; // 0
    int m_nextState; // 4
    Vector m_vecLocation; // 8, 12, 16
    QAngle m_angLocation; // 20, 24, 28
    char m_levelName[256]; // 32
    char m_landmarkName[256]; // 288
    char m_saveName[256]; // 544
    float m_flShortFrameTime; // 800
    bool m_activeGame; // 804
    bool m_bRememberLocation; // 805
    bool m_bBackgroundLevel; // 806
    bool m_bWaitingForConnection; // 807
};

#define INTERFACEVERSION_ISERVERPLUGINCALLBACKS "ISERVERPLUGINCALLBACKS002"

typedef void* (*CreateInterfaceFn)(const char* pName, int* pReturnCode);
typedef void* (*InstantiateInterfaceFn)();

struct InterfaceReg {
    InstantiateInterfaceFn m_CreateFn;
    const char* m_pName;
    InterfaceReg* m_pNext;
    static InterfaceReg* s_pInterfaceRegs;

    InterfaceReg(InstantiateInterfaceFn fn, const char* pName)
        : m_pName(pName)
    {
        m_CreateFn = fn;
        m_pNext = s_pInterfaceRegs;
        s_pInterfaceRegs = this;
    }
};

struct CPlugin {
    char m_szName[128]; // 0
    bool m_bDisable; // 128
    void* m_pPlugin; // 132
    int m_iPluginInterfaceVersion; // 136
    void* m_pPluginModule; // 140
};

struct CEventAction {
    const char* m_iTarget; // 0
    const char* m_iTargetInput; // 4
    const char* m_iParameter; // 8
    float m_flDelay; // 12
    int m_nTimesToFire; // 16
    int m_iIDStamp; // 20
    CEventAction* m_pNext; // 24
};

struct EventQueuePrioritizedEvent_t {
    float m_flFireTime; // 0
    char* m_iTarget; // 4
    char* m_iTargetInput; // 8
    int m_pActivator; // 12
    int m_pCaller; // 16
    int m_iOutputID; // 20
    int m_pEntTarget; // 24
    char m_VariantValue[20]; // 28
    EventQueuePrioritizedEvent_t* m_pNext; // 48
    EventQueuePrioritizedEvent_t* m_pPrev; // 52
};

struct CEventQueue {
    EventQueuePrioritizedEvent_t m_Events; // 0
    int m_iListCount; // 56
};

struct CEntInfo {
    void* m_pEntity; // 0
    int m_SerialNumber; // 4
    CEntInfo* m_pPrev; // 8
    CEntInfo* m_pNext; // 12
};

struct CEntInfo2 : CEntInfo {
    void* unk1; // 16
    void* unk2; // 20
};

typedef enum {
    DPT_Int = 0,
    DPT_Float,
    DPT_Vector,
    DPT_VectorXY,
    DPT_String,
    DPT_Array,
    DPT_DataTable,
    DPT_Int64,
    DPT_NUMSendPropTypes
} SendPropType;

struct SendProp;
struct RecvProp;
struct SendTable;

typedef void (*RecvVarProxyFn)(const void* pData, void* pStruct, void* pOut);
typedef void (*ArrayLengthRecvProxyFn)(void* pStruct, int objectID, int currentArrayLength);
typedef void (*DataTableRecvVarProxyFn)(const RecvProp* pProp, void** pOut, void* pData, int objectID);
typedef void (*SendVarProxyFn)(const SendProp* pProp, const void* pStructBase, const void* pData, void* pOut, int iElement, int objectID);
typedef int (*ArrayLengthSendProxyFn)(const void* pStruct, int objectID);
typedef void* (*SendTableProxyFn)(const SendProp* pProp, const void* pStructBase, const void* pData, void* pRecipients, int objectID);

struct RecvTable {
    RecvProp* m_pProps;
    int m_nProps;
    void* m_pDecoder;
    char* m_pNetTableName;
    bool m_bInitialized;
    bool m_bInMainList;
};

struct RecvProp {
    char* m_pVarName;
    SendPropType m_RecvType;
    int m_Flags;
    int m_StringBufferSize;
    bool m_bInsideArray;
    const void* m_pExtraData;
    RecvProp* m_pArrayProp;
    ArrayLengthRecvProxyFn m_ArrayLengthProxy;
    RecvVarProxyFn m_ProxyFn;
    DataTableRecvVarProxyFn m_DataTableProxyFn;
    RecvTable* m_pDataTable;
    int m_Offset;
    int m_ElementStride;
    int m_nElements;
    const char* m_pParentArrayPropName;
};

struct SendProp {
    void* VMT; // 0
    RecvProp* m_pMatchingRecvProp; // 4
    SendPropType m_Type; // 8
    int m_nBits; // 12
    float m_fLowValue; // 16
    float m_fHighValue; // 20
    SendProp* m_pArrayProp; // 24
    ArrayLengthSendProxyFn m_ArrayLengthProxy; // 28
    int m_nElements; // 32
    int m_ElementStride; // 36
    char* m_pExcludeDTName; // 40
    char* m_pParentArrayPropName; // 44
    char* m_pVarName; // 48
    float m_fHighLowMul; // 52
    int m_Flags; // 56
    SendVarProxyFn m_ProxyFn; // 60
    SendTableProxyFn m_DataTableProxyFn; // 64
    SendTable* m_pDataTable; // 68
    int m_Offset; // 72
    const void* m_pExtraData; // 76
};

struct SendProp2 {
    void* VMT; // 0
    RecvProp* m_pMatchingRecvProp; // 4
    SendPropType m_Type; // 8
    int m_nBits; // 12
    float m_fLowValue; // 16
    float m_fHighValue; // 20
    SendProp2* m_pArrayProp; // 24
    ArrayLengthSendProxyFn m_ArrayLengthProxy; // 28
    int m_nElements; // 32
    int m_ElementStride; // 36
    char* m_pExcludeDTName; // 40
    char* m_pParentArrayPropName; // 44
    char* m_pVarName; // 48
    float m_fHighLowMul; // 52
    char m_priority; // 56
    int m_Flags; // 60
    SendVarProxyFn m_ProxyFn; // 64
    SendTableProxyFn m_DataTableProxyFn; // 68
    SendTable* m_pDataTable; // 72
    int m_Offset; // 76
    const void* m_pExtraData; // 80
};

struct SendTable {
    SendProp* m_pProps;
    int m_nProps;
    char* m_pNetTableName;
    void* m_pPrecalc;
    bool m_bInitialized : 1;
    bool m_bHasBeenWritten : 1;
    bool m_bHasPropsEncodedAgainstCurrentTickCount : 1;
};

typedef void* (*CreateClientClassFn)(int entnum, int serialNum);
typedef void* (*CreateEventFn)();

struct ClientClass {
    CreateClientClassFn m_pCreateFn;
    CreateEventFn m_pCreateEventFn;
    char* m_pNetworkName;
    RecvTable* m_pRecvTable;
    ClientClass* m_pNext;
    int m_ClassID;
};

struct ServerClass {
    char* m_pNetworkName;
    SendTable* m_pTable;
    ServerClass* m_pNext;
    int m_ClassID;
    int m_InstanceBaselineIndex;
};

typedef enum _fieldtypes {
    FIELD_VOID = 0,
    FIELD_FLOAT,
    FIELD_STRING,
    FIELD_VECTOR,
    FIELD_QUATERNION,
    FIELD_INTEGER,
    FIELD_BOOLEAN,
    FIELD_SHORT,
    FIELD_CHARACTER,
    FIELD_COLOR32,
    FIELD_EMBEDDED,
    FIELD_CUSTOM,
    FIELD_CLASSPTR,
    FIELD_EHANDLE,
    FIELD_EDICT,
    FIELD_POSITION_VECTOR,
    FIELD_TIME,
    FIELD_TICK,
    FIELD_MODELNAME,
    FIELD_SOUNDNAME,
    FIELD_INPUT,
    FIELD_FUNCTION,
    FIELD_VMATRIX,
    FIELD_VMATRIX_WORLDSPACE,
    FIELD_MATRIX3X4_WORLDSPACE,
    FIELD_INTERVAL,
    FIELD_MODELINDEX,
    FIELD_MATERIALINDEX,
    FIELD_VECTOR2D,
    FIELD_TYPECOUNT
} fieldtype_t;

enum {
    TD_OFFSET_NORMAL = 0,
    TD_OFFSET_PACKED,
    TD_OFFSET_COUNT
};

struct inputdata_t;
typedef void (*inputfunc_t)(inputdata_t& data);

struct datamap_t;
struct typedescription_t {
    fieldtype_t fieldType; // 0
    const char* fieldName; // 4
    int fieldOffset[TD_OFFSET_COUNT]; // 8
    unsigned short fieldSize; // 16
    short flags; // 18
    const char* externalName; // 20
    void* pSaveRestoreOps; // 24
#ifndef _WIN32
    void* unk; // 28
#endif
    inputfunc_t inputFunc; // 28/32
    datamap_t* td; // 32/36
    int fieldSizeInBytes; // 36/40
    struct typedescription_t* override_field; // 40/44
    int override_count; // 44/48
    float fieldTolerance; // 48/52
};

struct datamap_t2;
struct typedescription_t2 {
    fieldtype_t fieldType; // 0
    const char* fieldName; // 4
    int fieldOffset; // 8
    unsigned short fieldSize; // 12
    short flags; // 14
    const char* externalName; // 16
    void* pSaveRestoreOps; // 20
#ifndef _WIN32
    void* unk1; // 24
#endif
    inputfunc_t inputFunc; // 24/28
    datamap_t2* td; // 28/32
    int fieldSizeInBytes; // 32/36
    struct typedescription_t2* override_field; // 36/40
    int override_count; // 40/44
    float fieldTolerance; // 44/48
    int flatOffset[TD_OFFSET_COUNT]; // 48/52
    unsigned short flatGroup; // 56/60
};

struct datamap_t {
    typedescription_t* dataDesc; // 0
    int dataNumFields; // 4
    char const* dataClassName; // 8
    datamap_t* baseMap; // 12
    bool chains_validated; // 16
    bool packed_offsets_computed; // 20
    int packed_size; // 24
};

struct datamap_t2 {
    typedescription_t2* dataDesc; // 0
    int dataNumFields; // 4
    char const* dataClassName; // 8
    datamap_t2* baseMap; // 12
    int m_nPackedSize; // 16
    void* m_pOptimizedDataMap; // 20
};

enum MapLoadType_t {
    MapLoad_NewGame = 0,
    MapLoad_LoadGame = 1,
    MapLoad_Transition = 2,
    MapLoad_Background = 3
};

#define FL_EDICT_FREE (1 << 1)

struct CBaseEdict {
    int m_fStateFlags; // 0
    int m_NetworkSerialNumber; // 4
    void* m_pNetworkable; // 8
    void* m_pUnk; // 12

    inline bool IsFree() const
    {
        return (m_fStateFlags & FL_EDICT_FREE) != 0;
    }
};

struct edict_t : CBaseEdict {
};

int ENTINDEX(edict_t* pEdict);
edict_t* INDEXENT(int iEdictNum);

struct CGlobalVarsBase {
    float realtime; // 0
    int framecount; // 4
    float absoluteframetime; // 8
    float curtime; // 12
    float frametime; // 16
    int maxClients; // 20
    int tickcount; // 24
    float interval_per_tick; // 28
    float interpolation_amount; // 32
    int simTicksThisFrame; // 36
    int network_protocol; // 40
    void* pSaveData; // 44
    bool m_bClient; // 48
    int nTimestampNetworkingBase; // 52
    int nTimestampRandomizeWindow; // 56
};

struct CGlobalVars : CGlobalVarsBase {
    char* mapname; // 60
    int mapversion; // 64
    char* startspot; // 68
    MapLoadType_t eLoadType; // 72
    char bMapLoadFailed; // 76
    char deathmatch; // 77
    char coop; // 78
    char teamplay; // 79
    int maxEntities; // 80
    int serverCount; // 84
    edict_t* pEdicts; // 88
};

enum JoystickAxis_t {
    JOY_AXIS_X = 0,
    JOY_AXIS_Y,
    JOY_AXIS_Z,
    JOY_AXIS_R,
    JOY_AXIS_U,
    JOY_AXIS_V,
    MAX_JOYSTICK_AXES,
};

typedef struct {
    unsigned int AxisFlags; // 0
    unsigned int AxisMap; // 4
    unsigned int ControlMap; // 8
} joy_axis_t;

struct CameraThirdData_t {
    float m_flPitch; // 0
    float m_flYaw; // 4
    float m_flDist; // 8
    float m_flLag; // 12
    Vector m_vecHullMin; // 16, 20, 24
    Vector m_vecHullMax; // 28, 32, 36
};

typedef unsigned long CRC32_t;

class CVerifiedUserCmd {
public:
    CUserCmd m_cmd;
    CRC32_t m_crc;
};

struct PerUserInput_t {
    float m_flAccumulatedMouseXMovement; // ?
    float m_flAccumulatedMouseYMovement; // ?
    float m_flPreviousMouseXPosition; // ?
    float m_flPreviousMouseYPosition; // ?
    float m_flRemainingJoystickSampleTime; // ?
    float m_flKeyboardSampleTime; // 12
    float m_flSpinFrameTime; // ?
    float m_flSpinRate; // ?
    float m_flLastYawAngle; // ?
    joy_axis_t m_rgAxes[MAX_JOYSTICK_AXES]; // ???
    bool m_fCameraInterceptingMouse; // ?
    bool m_fCameraInThirdPerson; // ?
    bool m_fCameraMovingWithMouse; // ?
    Vector m_vecCameraOffset; // 104, 108, 112
    bool m_fCameraDistanceMove; // 116
    int m_nCameraOldX; // 120
    int m_nCameraOldY; // 124
    int m_nCameraX; // 128
    int m_nCameraY; // 132
    bool m_CameraIsOrthographic; // 136
    QAngle m_angPreviousViewAngles; // 140, 144, 148
    QAngle m_angPreviousViewAnglesTilt; // 152, 156, 160
    float m_flLastForwardMove; // 164
    int m_nClearInputState; // 168
    CUserCmd* m_pCommands; // 172
    CVerifiedUserCmd* m_pVerifiedCommands; // 176
    unsigned long m_hSelectedWeapon; // 180 CHandle<C_BaseCombatWeapon>
    CameraThirdData_t* m_pCameraThirdData; // 184
    int m_nCamCommand; // 188
};

struct kbutton_t {
    struct Split_t {
        int down[2];
        int state;
    };

    Split_t& GetPerUser(int nSlot = -1);
    Split_t m_PerUser[2];
};

enum TOGGLE_STATE {
    TS_AT_TOP,
    TS_AT_BOTTOM,
    TS_GOING_UP,
    TS_GOING_DOWN
};

typedef enum {
    USE_OFF = 0,
    USE_ON = 1,
    USE_SET = 2,
    USE_TOGGLE = 3
} USE_TYPE;

class IGameEvent {
public:
    virtual ~IGameEvent() = default;
    virtual const char* GetName() const = 0;
    virtual bool IsReliable() const = 0;
    virtual bool IsLocal() const = 0;
    virtual bool IsEmpty(const char* key = 0) = 0;
    virtual bool GetBool(const char* key = 0, bool default_value = false) = 0;
    virtual int GetInt(const char* key = 0, int default_value = 0) = 0;
    virtual float GetFloat(const char* key = 0, float default_value = 0.0f) = 0;
    virtual const char* GetString(const char* key = 0, const char* default_value = "") = 0;
    virtual void SetBool(const char* key, bool value) = 0;
    virtual void SetInt(const char* key, int value) = 0;
    virtual void SetFloat(const char* key, float value) = 0;
    virtual void SetString(const char* key, const char* value) = 0;
};

class IGameEventListener2 {
public:
    virtual ~IGameEventListener2() = default;
    virtual void FireGameEvent(IGameEvent* event) = 0;
    virtual int GetEventDebugID() = 0;
};

static const char* EVENTS[] = {
    "player_spawn_blue",
    "player_spawn_orange"
};

struct cmdalias_t {
    cmdalias_t* next;
    char name[32];
    char* value;
};

struct GameOverlayActivated_t {
    uint8_t m_bActive;
};

enum PaintMode_t {
    PAINT_UIPANELS = (1 << 0),
    PAINT_INGAMEPANELS = (1 << 1),
};
