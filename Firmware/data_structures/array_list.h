#pragma 
/**
 * @file array_list.h
 * @brief A contiguous array list using using indices (not pointers).
 */

#include "util/span.h"

#include <algorithm>
#include <optional>

namespace data_structures {


    /*  Single-ended List main API: 'retrieve' means return a Node& (usually mutable)

        - Retrieve an element by list logical position (1st=Head, 2nd, 3rd, etc)
        - Retrieve an element by comparator(T elem) (search, may return invalid)
        - Retrieve the next element, given an element reference
        - Iterate over all elements

        - Delete a selected element
        - Clear the list (just repeat the constructor initialization)

        - Insert an element after a given element
        - Prepend an element to the head of the list
        - Append an element to the end of the list

        Possible API: retrieve relative to end using negative index?
    */


// Class ArrayList (takes storage as input)
template <typename T, typename IndexType = uint16_t>
class ArrayList {
private:
    static constexpr IndexType kInvalidIndex = std::numeric_limits<IndexType>::max();

    bool isValidIndex(IndexType index) const noexcept {
        return index < capacity();
    }

    // If the input is valid, returns it as-is
    // Otherwise, returns kInvalidIndex
    // TODO: name could be more clear about the semantics
    constexpr IndexType validateIndex(IndexType input) const noexcept {
        return isValidIndex(input) ? input : kInvalidIndex;
    }

    /*
     * @brief An element in the single-ended linked list, using an array index instead
     *        of a pointer.
     */
    struct Node {
        T value{};
        IndexType next{kInvalidIndex};
    };

public:
    /*
     * @brief Representation of a Node which also stores a link to the previous element.
     *        This is the primary way of representing an element in the list for the public
     *        APIs, since it is more capable than having a (single-ended) Node object
     *        (Self-deletion requires knowing the index of the predecessor Node).
     */
    struct NodeView {
        // TODO: if T is large, copying it just to get a NodeView as a search function
        // result is annoying / inefficient. Same applies to its use in APIs like
        // insertBeforeSelf, insertAfterSelf.
        //
        // Idea to fix this:
        // - NodeView holds a private `Node* const` and the `prev` and `self` members.
        // - public `T& value()` accessors that return Node->value
        // - private IndexType next() accessor that returns Node->next

        T value{};

    private:
        friend class ArrayList<T, IndexType>;

        NodeView(T node_val, IndexType prev_idx, IndexType self_idx, IndexType next_idx)
            : value{node_val}, prev{prev_idx}, self{self_idx}, next{next_idx} {}

        IndexType prev{kInvalidIndex};
        IndexType self{kInvalidIndex};
        IndexType next{kInvalidIndex};
    };

    // numNodesFromSizeBytes(storage.size()) nodes will be constructed. It is the user's
    // responsibility to ensure that the storage is large enough.
    explicit constexpr ArrayList(Span<Node> storage) noexcept
        : kCapacity(numNodesFromSizeBytes(storage.size())),
          size_(0U),
          head_index_(kInvalidIndex),
          free_list_head_(0U),
          storage_(storage) 
    {
        doAllInitializations();
    }

    // TODO: could size calculations be moved to details namespace?
    // How much storage is required for the given capacity?
    constexpr size_t numBytesRequired(size_t num_nodes) const noexcept {
        return sizeof(Node) * num_nodes;
    }

    // How many nodes can we fit in the provided storage?
    constexpr size_t numNodesFromSizeBytes(size_t size_bytes) const noexcept {
        return size_bytes / sizeof(Node);
    }

/******************************************************************************
 *  Public API
 *  - Exposes NodeView rather than indexes or Node&
 ******************************************************************************/
    // Forward declaration
    // TODO: can we have a way to use iterators starting from a NodeView?
    template <typename Node, typename IndexType>
    class Iterator;

    Iterator begin();
    Iterator end();

    constexpr size_t capacity() const noexcept;
    size_t size() const noexcept;
    bool numFreeNodes() const noexcept;

    bool pushFront(const T& value) noexcept;
    bool pushBack(const T& value) noexcept;
    std::optional<T> popFront() noexcept;
    std::optional<T> popBack() noexcept;

    std::optional<NodeView> findNode(const T& search_key) noexcept;
    std::optional<NodeView> nextNode(const NodeView& current_node) noexcept;
    std::optional<NodeView> nodeFromSeek(NodeView start_node, size_t distance) noexcept;
    std::optional<NodeView> nodeAtLogicalPosition(size_t position) noexcept;

    bool insertBeforeSelf(const T& value, NodeView node) noexcept;
    bool insertAfterSelf(const T& value, NodeView node) noexcept;

    bool deleteNode(NodeView& node) noexcept;
    void clear() noexcept;


/******************************************************************************
 *  Public API method implementations
 ******************************************************************************/
 private:

    constexpr size_t capacity() const noexcept {
        return kCapacity;
    }

    size_t size() const noexcept {
        return num_allocated_;
    }

    bool numFreeNodes() const noexcept {
        return kCapacity - num_allocated_;
    }

    bool pushFront(const T& value) noexcept {
        const std::optional<IndexType> index = allocateNode();
        if (!index.hasValue() || !isValidIndex(index.getValue())) {
            return false; // TODO: log errors
        }

        Node& new_node = storage_[index];
        new_node.value = value;
        new_node.next = user_list_head_;
        user_list_head_ = index;
        return true;
    }

    // TODO: Consider more efficient implementations of pushBack() for a single-ended linked list.
    //  - Keep track of a new `user_list_tail_`, which is the index of the last Node (or kInvalidIndex)
    //  - This feature would also enable a single cheap popBack(), but repeated popBack() would still be inefficient.
    bool pushBack(const T& value) noexcept {
        const std::optional<IndexType> index = allocateNode();
        if (!index.hasValue() || !isValidIndex(index.getValue())) {
            return false; // TODO: log errors
        }

        Node& new_node = storage_[index];
        new_node.value = value;
        new_node.next = user_list_head_;
        user_list_head_ = index;
        return true;
    }

    std::optional<T> popFront() noexcept {
        if (!isValidIndex(user_list_head_)) {
            return util::nullopt;
        }

        const T value = storage_[user_list_head_].value;
        deleteHeadNode();
        return value;
    }

    std::optional<T> popBack() noexcept {
        return util::nullopt;
    }

    // TODO: might be OK to remove the separate Impl? But it is desirable to separate the user API
    // from the implementation details.
    std::optional<NodeView> findNode(const T& search_key) noexcept {
        return findNodeImpl(search_key);
    }

    std::optional<NodeView> nextNode(const NodeView& current_node) noexcept {
        if (!isValidIndex(current_node.self) || !isValidIndex(current_node.next)) {
            return util::nullopt;
        }
        const Node& next_node{storage_[current_node.next]};
        return NodeView{next_node.value, current_node.self, current_node.next, validateIndex(next_node.next)};
    }


    // TODO: should this return a reference to the inserted item?
    bool insertBeforeSelf(const T& value, NodeView node) noexcept {
        const IndexType predecessor_index = node.prev;
        return insertAfterIndex(value, predecessor_index);
    }

    // TODO: should this return a View to the inserted item?
    bool insertAfterSelf(const T& value, NodeView node) noexcept {
        const IndexType predecessor_index = node.self;
        return insertAfterIndex(value, predecessor_index);
    }

    // Follow up to `distance` number of Node->next links from `start_node` and return a NodeView
    // of the Node at that location, or std::nullopt if the list end is reached instead.
    std::optional<NodeView> nodeFromSeek(NodeView start_node, size_t distance) noexcept {
        return nodeViewFromIndexSeek(start_node.self, distance);
    }

    // Same as above but starting from the very first Node in the list
    std::optional<NodeView> nodeAtLogicalPosition(size_t position) noexcept {
        return nodeViewFromIndexSeek(user_list_head_, distance);
    }

    /* Delete a node from the user list, using a NodeView
    * (The node will be unlinked from neighbors, then garbage collected)
    */
    bool deleteNode(NodeView& node) noexcept {
        return deleteNodeImpl(node);
    }

    void clear() noexcept {
        doAllInitializations();
    }

    // TODO: any way to make this safer (increment past end, or other OOB access)
    template <typename Node, typename IndexType>
    class Iterator {
        Iterator(Span<Node> storage, IndexType index) : storage_{storage}, index_{index};

        Iterator& operator++() noexcept { 
            index_ = storage_[index_].next;
            return *this; 
        }

        const Node::T& operator*() const noexcept {
            return &storage_[index_].value;
        }

        Node::T& operator*() const noexcept {
            return &storage_[index_].value;
        }

        bool operator!=(const Iterator& other) const noexcept { 
            return index_ != other.index_; 
        }

    private:
        const Span<Node> storage_{};
        IndexType index_{}
    };

    Iterator::begin() { return Iterator{ storage_, user_list_head_ }; }
    Iterator::end()   { return Iterator{ storage_, kInvalidIndex }; }


/******************************************************************************
 *  Private method implementations
 *  - These may use indexes and Node& directly instead of NodeView
 ******************************************************************************/
    /*
     * @brief   Initializes the free list so that all nodes are free
     * @details The nodes are linked as: null <- 0 <- 1 <- 2 <- ... <- N-1
     * @warning This will (partially) reset the data structure.
     */
    void initializeFreeList() noexcept {
        // Initialize the free list: storage_[0] is the free list head
        free_list_head_ = 0U;
        storage_[free_list_head_].next = kInvalidIndex;
        for (IndexType i = 1U; i < kCapacity; i++) {
            storage_[i].next = i - 1U;
        }
    }

    /*
     * @brief   Initializes the user list and allocation count (empty list)
     * @warning This will (partially) reset the data structure.
     */
    void initializeUserList() noexcept {
        user_list_head_ = kInvalidIndex;
        num_allocated_ = 0U;
    }

    /*
     * @brief   Puts all nodes in the free list and resets the user list and allocation count.
     * @warning This will reset the data structure.
     */
    void doAllInitializations() noexcept {
        initializeFreeList();
        initializeUserList();
    }

    /*
     * @brief   Allocates a new node from the free list, if there is one.
     * @details The returned node does not point to anything.
     * @return  The index of the allocated node, or std::nullopt if there are no free nodes.
     */
    std::optional<IndexType> allocateNode() noexcept {
        if (!isValidIndex(free_list_head_)) {
            // Either there are no free nodes, or the free_list_head_ is corrupted
            // TODO: signal / log this somehow
            return std::nullopt;
        }

        IndexType allocated_index = free_list_head_;
        Node& allocated = storage_[allocated_index];
        free_list_head_ = validateIndex(allocated.next);
        allocated.next = kInvalidIndex;
        num_allocated_++;
        return allocated_index;
    }

    /*
     * @brief  Garbage collects a node after is has been unlinked, returning it to the head of the free list.
     * @pre    The node has already been unlinked from the user list. The node's `next` field is allowed to
     *         contain any value.
     */
     void deallocateNode(IndexType index) noexcept {
        if (!isValidIndex(index)) { // TODO: signal / log this somehow
            return;
        }

        Node& next_free_list_head_ = storage_[index]; // will be the free_list_head after return

        if (!isValidIndex(free_list_head_)) {
            // The free list is empty. index becomes the new head, and it points to nothing.
            next_free_list_head_.next = kInvalidIndex;
            free_list_head_ = index;
        } else {
            // The free list is non-empty. index becomes the new head, and it points to the old head.
            next_free_list_head_.next = free_list_head_;
            free_list_head_ = index;
        }

        num_allocated_--;
    }

    /* Before: A->B
     * Call:   insertAfterIndex(value, A);
     * After:  A->new_node(value)->B
     */
    bool insertAfterIndex(const T& value, IndexType predecessor_index) noexcept {
        if (!isValidIndex(predecessor_index)) { // Node A is invalid
            return false;
        }

        std::optional<IndexType> new_node_index = allocateNode();
        if (!new_node_index.hasValue()) {
            // TODO: log allocation failure
            return false;
        }

        Node& predecessor = storage_[predecessor_index];
        Node& new_node = storage_[new_node_index.value()];

        new_node.next = validateIndex(predecessor.next);
        predecessor.next = new_node_index.value();
        return true;
    }

    /* Delete the first node (head) of the user list
     * (The node will be unlinked from neighbors, then garbage collected)
     */
     bool deleteHeadNode() noexcept {
        if (!isValidIndex(user_list_head_)) {
            return false;
        }

        Node& old_user_head = storage_[user_list_head_];
        if (isValidIndex(old_user_head.next)) {
            user_list_head_ = old_user_head.next);
        } else {
            user_list_head_ = kInvalidIndex;
        }

        deallocateNode(old_user_head);
        return true;
    }

    /* Given the index of Node A, where A->B (and potentially B->C), delete B
     * (The node will be unlinked from neighbors, then garbage collected)
     */
    bool deleteSuccessorNode(IndexType predecessor_index) noexcept {
        if (!isValidIndex(predecessor_index)) { // Node A is invalid
            return false;
        }

        Node& predecessor = storage_[predecessor_index];
        if (!isValidIndex(predecessor.next)) { // Node B is invalid
            return false;
        }

        const IndexType victim_index = predecessor.next;
        Node& victim = storage_[victim_index];
        if (isValidIndex(victim.next)) {      // Node C is _valid_
            predecessor.next = victim.next;   // A->C
        } else {
            predecessor.next = kInvalidIndex; // A->null
        }

        deallocateNode(victim_index);
        return true;
    }

    /* Delete a node from the user list, using a NodeView
     * (The node will be unlinked from neighbors, then garbage collected)
     */
    bool deleteNodeImpl(NodeView& node) noexcept {
        if (!isValidIndex(node.self)) {
            return false; // Should not happen
        }

        if (node.self == user_list_head_) {
            return deleteHeadNode();
        }

        if (isValidIndex(node.prev)) {
            return deleteSuccessorNode(node.prev);
        }

        // Shouldn't happen
        // TODO: log this
        return false;
    }

    // Returns the first matching node in case of duplicate key values.
    // TODO: consider guarding against accidental cycles in the list
    std::optional<NodeView> findNodeImpl(const T& search_key) {
        // Empty list case
        if (!isValidIndex(user_list_head_)) {
            return util::nullopt;
        }
        // Handle the case where the head node contains the search key
        {
            const Node& head = storage_[user_list_head_];
            if (head.value == search_key) {
                return NodeView{head.value, kInvalidIndex, user_list_head_, validateIndex(head.next)};
            }
        }
        
        // Search the rest of the list, looking for the search key one Node ahead, so that
        // we will have the predecessor index for the found node.
        IndexType current_index = user_list_head_;
        IndexType next_index = user_list_head_.next;

        while (isValidIndex(next_index)) {
            const Node& next_node = &storage_[next_index];
            if (next_node.value == search_key) {
                return NodeView{next_node.value, current_index, next_index, validateIndex(next_node.next)};
            } else {
                current_index = next_index;
                next_index = next_node.next;
            }
        }
        return util::nullopt;
    }

    // Starting at the node pointed to by `start_index`, follow up to `distance` number
    // of Node->next links and return a NodeView representing the Node at that position,
    // or return std::nullopt if the list end is reached.
    std::optional<NodeView> nodeViewFromIndexSeek(IndexType start_index, size_t distance) {
        if (!isValidIndex(start_index)) {
            return util::nullopt; // Empty list
        }

        IndexType prev_index{kInvalidIndex};
        IndexType curr_index{start_index};

        // Iterate until the desired node is at curr_index, and the predecessor is at prev_index,
        // or we reach the end of the list.
        while (for size_t i = 0U; i < distance; i++) {
            const IndexType next_index{storage_[curr_index].next};
            if (!isValidIndex(next_index)) {
                return util::nullopt; // error: distance > this->size();
            }
            prev_index = curr_index;
            curr_index = next_index;
        }

        return NodeView{storage_[curr_index].value, prev_index, curr_index, storage_[curr_index].next}
    }

    const IndexType kCapacity;
    IndexType num_allocated_;  // Number of nodes in the user data list. Managed by allocateNode / deallocateNode;
                               // the "delete" methods find the node to delete while deallocate does the bookkeping
    IndexType user_list_head_; // Index of the first node in the user data list
    IndexType free_list_head_; // Index of the first node in the free node list
    Span<Node> storage_;
};

// Class StaticArrayList (reuses ArrayList, but provides it's own statically-allocated internal storage)

} // namespace data_structures