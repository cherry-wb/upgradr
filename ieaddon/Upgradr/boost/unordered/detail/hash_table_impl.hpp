
// Copyright (C) 2003-2004 Jeremy B. Maitin-Shepard.
// Copyright (C) 2005-2006 Daniel James
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if BOOST_UNORDERED_HASH_EQUIVALENT
#define HASH_TABLE hash_table_equivalent_keys
#define HASH_TABLE_DATA hash_table_data_equivalent_keys
#define HASH_ITERATOR hash_iterator_equivalent_keys
#define HASH_CONST_ITERATOR hash_const_iterator_equivalent_keys
#define HASH_LOCAL_ITERATOR hash_local_iterator_equivalent_keys
#define HASH_CONST_LOCAL_ITERATOR hash_const_local_iterator_equivalent_keys
#else
#define HASH_TABLE hash_table_unique_keys
#define HASH_TABLE_DATA hash_table_data_unique_keys
#define HASH_ITERATOR hash_iterator_unique_keys
#define HASH_CONST_ITERATOR hash_const_iterator_unique_keys
#define HASH_LOCAL_ITERATOR hash_local_iterator_unique_keys
#define HASH_CONST_LOCAL_ITERATOR hash_const_local_iterator_unique_keys
#endif

namespace boost {
    namespace unordered_detail {

        //
        // Hash Table Data
        //
        // Responsible for managing the hash buckets.

        template <class Alloc>
        class HASH_TABLE_DATA
        {
        public:
            class node;
            class node_base;
            class bucket;
            typedef std::size_t size_type;

            typedef Alloc value_allocator;

            typedef BOOST_DEDUCED_TYPENAME
                boost::unordered_detail::rebind_wrap<Alloc, node>::type
                node_allocator;
            typedef BOOST_DEDUCED_TYPENAME
                boost::unordered_detail::rebind_wrap<Alloc, node_base>::type
                node_base_allocator;
            typedef BOOST_DEDUCED_TYPENAME
                boost::unordered_detail::rebind_wrap<Alloc, bucket>::type
                bucket_allocator;

            typedef BOOST_DEDUCED_TYPENAME allocator_value_type<Alloc>::type value_type;
            typedef BOOST_DEDUCED_TYPENAME allocator_pointer<node_allocator>::type node_ptr;
            typedef BOOST_DEDUCED_TYPENAME allocator_pointer<bucket_allocator>::type bucket_ptr;
            typedef BOOST_DEDUCED_TYPENAME allocator_reference<value_allocator>::type reference;

            // Hash Bucket
            //
            // all no throw

            class bucket
            {
                bucket& operator=(bucket const&);
            public:
                bucket_ptr next_;

                bucket() : next_()
                {
                    BOOST_HASH_MSVC_RESET_PTR(next_);
                }

                bucket(bucket const& x) : next_(x.next_)
                {
                    // Only copy construct when allocating.
                    BOOST_ASSERT(!x.next_);
                }

                bool empty() const
                {
                    return !this->next_;
                }
            };

            // Hash Node
            //
            // all no throw

            class node_base : public bucket
            {
#if BOOST_UNORDERED_HASH_EQUIVALENT
            public:
                node_base() : group_prev_()
                {
                    BOOST_HASH_MSVC_RESET_PTR(group_prev_);
                }

                bucket_ptr group_prev_;
#endif
            };

            class node : public node_base
            {
            public:
                node(value_type const& v) : node_base(), value_(v) {}

                value_type value_;
            };

            // node_constructor
            //
            // Used to construct nodes in an exception safe manner.

            struct allocators
            {
                node_allocator node_alloc_;
                bucket_allocator bucket_alloc_;
                value_allocator value_alloc_;
                node_base_allocator node_base_alloc_;

                allocators(node_allocator const& a)
                    : node_alloc_(a), bucket_alloc_(a),
                    value_alloc_(a), node_base_alloc_(a)
                {}

                void destroy(bucket_ptr ptr)
                {
                    node_ptr n(node_alloc_.address(static_cast<node&>(*ptr)));
                    value_alloc_.destroy(value_alloc_.address(n->value_));
                    node_base_alloc_.destroy(node_base_alloc_.address(*n));
                    node_alloc_.deallocate(n, 1);
                }
            };

            class node_constructor
            {
                allocators& allocators_;
                node_base_allocator node_base_alloc_;
                value_allocator value_alloc_;

                node_ptr ptr_;
                bool value_constructed_;
                bool node_base_constructed_;

            public:

                node_constructor(allocators& a)
                    : allocators_(a), node_base_alloc_(a.node_alloc_),
                    value_alloc_(a.node_alloc_),
                    ptr_(), value_constructed_(false), node_base_constructed_(false)
                {
                    BOOST_HASH_MSVC_RESET_PTR(ptr_);
                }

                ~node_constructor()
                {
                    if (ptr_) {
                        if (value_constructed_)
                            value_alloc_.destroy(
                                value_alloc_.address(ptr_->value_));
                        if (node_base_constructed_)
                            node_base_alloc_.destroy(
                                node_base_alloc_.address(*ptr_));

                        allocators_.node_alloc_.deallocate(ptr_, 1);
                    }
                }

                template <class V>
                void construct(V const& v)
                {
                    BOOST_ASSERT(!ptr_);
                    value_constructed_ = node_base_constructed_ = false;

                    ptr_ = allocators_.node_alloc_.allocate(1);

                    node_base_alloc_.construct(node_base_alloc_.address(
                            *ptr_), node_base());
                    node_base_constructed_ = true;

                    value_alloc_.construct(value_alloc_.address(
                            ptr_->value_), v);
                    value_constructed_ = true;
                }

                node_ptr get() const
                {
                    BOOST_ASSERT(ptr_);
                    return ptr_;
                }

                // no throw
                bucket_ptr release()
                {
                    node_ptr p = ptr_;
                    unordered_detail::reset(ptr_);
                    return allocators_.bucket_alloc_.address(*p);
                }

            private:
                node_constructor(node_constructor const&);
                node_constructor& operator=(node_constructor const&);
            };

            // Methods for navigating groups of elements with equal keys.

#if BOOST_UNORDERED_HASH_EQUIVALENT
            static bucket_ptr& prev_in_group(bucket_ptr p) {
                return static_cast<node&>(*p).group_prev_;
            }

            // pre: Must be pointing to the first node in a group.
            static bucket_ptr last_in_group(bucket_ptr p) {
                BOOST_ASSERT(p && p != prev_in_group(p)->next_);
                return prev_in_group(p);
            }

            // pre: Must be pointing to the first node in a group.
            static bucket_ptr& next_group(bucket_ptr p) {
                BOOST_ASSERT(p && p != prev_in_group(p)->next_);
                return prev_in_group(p)->next_;
            }
#else
            static bucket_ptr last_in_group(bucket_ptr p) {
                return p;
            }

            static bucket_ptr& next_group(bucket_ptr p) {
                BOOST_ASSERT(p);
                return p->next_;
            }
#endif

            // pre: Must be pointing to a node
            static node& get_node(bucket_ptr p) {
                BOOST_ASSERT(p);
                return static_cast<node&>(*p);
            }

            // pre: Must be pointing to a node
            static reference get_value(bucket_ptr p) {
                BOOST_ASSERT(p);
                return static_cast<node&>(*p).value_;
            }

            class local_iterator_base
            {
            public:
                bucket_ptr node_pointer_;

                local_iterator_base()
                    : node_pointer_()
                {
                    BOOST_HASH_MSVC_RESET_PTR(node_pointer_);
                }

                explicit local_iterator_base(bucket_ptr n)
                    : node_pointer_(n) {}

                bool not_finished() const
                {
                    return node_pointer_ ? true : false;
                }

                bool operator==(local_iterator_base const& x) const
                {
                    return node_pointer_ == x.node_pointer_;
                }

                bool operator!=(local_iterator_base const& x) const
                {
                    return node_pointer_ != x.node_pointer_;
                }

                reference operator*() const
                {
                    return get_value(node_pointer_);
                }

                void increment()
                {
                    BOOST_ASSERT(node_pointer_);
                    node_pointer_ = node_pointer_->next_;
                }

                void next_group()
                {
                    node_pointer_ = HASH_TABLE_DATA::next_group(node_pointer_);
                }
            };

            class iterator_base
            {
            public:
                bucket_ptr bucket_;
                local_iterator_base local_;

                iterator_base()
                    : bucket_(), local_() {}

                explicit iterator_base(bucket_ptr b)
                    : bucket_(b), local_(b->next_) {}

                iterator_base(bucket_ptr b, bucket_ptr n)
                    : bucket_(b), local_(n) {}

                iterator_base(bucket_ptr b, local_iterator_base const& it)
                    : bucket_(b), local_(it) {}

                bool operator==(iterator_base const& x) const
                {
                    return local_ == x.local_;
                }

                bool operator!=(iterator_base const& x) const
                {
                    return local_ != x.local_;
                }

                reference operator*() const
                {
                    return *local_;
                }

                void increment()
                {
                    BOOST_ASSERT(bucket_);
                    local_.increment();

                    while (!local_.node_pointer_) {
                        ++bucket_;
                        local_ = local_iterator_base(bucket_->next_);
                    }
                }
            };

            // Member Variables

            allocators allocators_;
            bucket_ptr buckets_;
            size_type bucket_count_;
            bucket_ptr cached_begin_bucket_;
            size_type size_;

            // Constructors/Deconstructor

            HASH_TABLE_DATA(size_type n, node_allocator const& a)
              : allocators_(a),
                buckets_(), bucket_count_(next_prime(n)),
                cached_begin_bucket_(), size_(0)
            {
                // The array constructor will clean up in the event of an
                // exception.
                allocator_array_constructor<bucket_allocator>
                    constructor(allocators_.bucket_alloc_);

                // Creates an extra bucket to act as a sentinel.
                constructor.construct(bucket(), bucket_count_ + 1);

                cached_begin_bucket_ = constructor.get() + bucket_count_;

                // Set up the sentinel.
                cached_begin_bucket_->next_ = cached_begin_bucket_;

                // Only release the buckets once everything is successfully
                // done.
                buckets_ = constructor.release();
            }

            HASH_TABLE_DATA(HASH_TABLE_DATA const& h, size_type n)
              : allocators_(h.allocators_),
                buckets_(), bucket_count_(next_prime(n)),
                cached_begin_bucket_(), size_(0)
            {
                // The array constructor will clean up in the event of an
                // exception.
                allocator_array_constructor<bucket_allocator>
                    constructor(allocators_.bucket_alloc_);

                // Creates an extra bucket to act as a sentinel.
                constructor.construct(bucket(), bucket_count_ + 1);

                cached_begin_bucket_ = constructor.get() + bucket_count_;

                // Set up the sentinel
                cached_begin_bucket_->next_ = cached_begin_bucket_;

                // Only release the buckets once everything is successfully
                // done.
                buckets_ = constructor.release();
            }

            // no throw
            ~HASH_TABLE_DATA()
            {
                if(buckets_) {
                    bucket_ptr begin = cached_begin_bucket_;
                    bucket_ptr end = buckets_ + bucket_count_;
                    while(begin != end) {
                        clear_bucket(begin);
                        ++begin;
                    }

                    for(size_type i2 = 0; i2 < bucket_count_ + 1; ++i2)
                        allocators_.bucket_alloc_.destroy(buckets_ + i2);

                    allocators_.bucket_alloc_.deallocate(buckets_, bucket_count_ + 1);
                }
            }

        private:

            HASH_TABLE_DATA(HASH_TABLE_DATA const&);
            HASH_TABLE_DATA& operator=(HASH_TABLE_DATA const&);

        public:

            // no throw
            void swap(HASH_TABLE_DATA& other)
            {
                std::swap(buckets_, other.buckets_);
                std::swap(bucket_count_, other.bucket_count_);
                std::swap(cached_begin_bucket_, other.cached_begin_bucket_);
                std::swap(size_, other.size_);
            }

            // Return the bucket index for a hashed value.
            //
            // no throw
            size_type index_from_hash(size_type hashed) const
            {
                return hashed % bucket_count_;
            }

            // Begin & End
            //
            // no throw

            iterator_base begin() const
            {
                return size_
                    ? iterator_base(cached_begin_bucket_)
                    : end();
            }

            iterator_base end() const
            {
                return iterator_base(buckets_ + bucket_count_);
            }

            local_iterator_base begin(size_type n) const
            {
                return local_iterator_base(buckets_[n].next_);
            }

            local_iterator_base end(size_type) const
            {
                return local_iterator_base();
            }

            local_iterator_base begin(bucket_ptr b) const
            {
                return local_iterator_base(b->next_);
            }

            // Bucket Size

            // no throw
            size_type node_count(local_iterator_base it) const
            {
                size_type count = 0;
                while(it.not_finished()) {
                    ++count;
                    it.increment();
                }
                return count;
            }

            size_type node_count(local_iterator_base it1,
                    local_iterator_base it2) const
            {
                size_type count = 0;
                while(it1 != it2) {
                    ++count;
                    it1.increment();
                }
                return count;
            }

            size_type bucket_size(size_type n) const
            {
                return node_count(begin(n));
            }

#if BOOST_UNORDERED_HASH_EQUIVALENT
            size_type group_count(local_iterator_base pos) const
            {
                size_type count = 0;
                bucket_ptr it = pos.node_pointer_;
                bucket_ptr first = it;
                do {
                    ++count;
                    it = prev_in_group(it);
                } while (it != first); // throws, strong
                return count;
            }
#else
            size_type group_count(local_iterator_base) const
            {
                return 1;
            }
#endif

            // get_for_erase
            //
            // Find the pointer to a node, for use when erasing.
            //
            // no throw

#if BOOST_UNORDERED_HASH_EQUIVALENT
            bucket_ptr* get_for_erase(iterator_base r) const
            {
                bucket_ptr pos = r.local_.node_pointer_;

                // If the element isn't the first in its group, then
                // the link to it will be found in the previous element
                // in the group.
                bucket_ptr* it = &prev_in_group(pos)->next_;
                if(*it == pos) return it;

                // The element is the first in its group, so just
                // need to check the first elements.
                it = &r.bucket_->next_;
                while(*it != pos) it = &HASH_TABLE_DATA::next_group(*it);
                return it;
            }
#else
            bucket_ptr* get_for_erase(iterator_base r) const
            {
                bucket_ptr pos = r.local_.node_pointer_;
                bucket_ptr* it = &r.bucket_->next_;
                while(*it != pos) it = &(*it)->next_;
                return it;
            }
#endif

            // Link/Unlink/Move Node
            //
            // For adding nodes to buckets, removing them and moving them to a
            // new bucket.
            //
            // no throw

#if BOOST_UNORDERED_HASH_EQUIVALENT
            void link_node(bucket_ptr n, local_iterator_base pos)
            {
                node& node_ref = get_node(n);
                node& pos_ref = get_node(pos.node_pointer_);
                node_ref.next_ = pos_ref.group_prev_->next_;
                node_ref.group_prev_ = pos_ref.group_prev_;
                pos_ref.group_prev_->next_ = n;
                pos_ref.group_prev_ = n;
                ++size_;
            }

            void link_node(bucket_ptr n, bucket_ptr base)
            {
                node& node_ref = get_node(n);
                node_ref.next_ = base->next_;
                node_ref.group_prev_ = n;
                base->next_ = n;
                ++size_;
                if(base < cached_begin_bucket_) cached_begin_bucket_ = base;
            }

            void link_group(bucket_ptr n, bucket_ptr base, size_type count)
            {
                node& node_ref = get_node(n);
                node& last_ref = get_node(node_ref.group_prev_);
                last_ref.next_ = base->next_;
                base->next_ = n;
                size_ += count;
                if(base < cached_begin_bucket_) cached_begin_bucket_ = base;
            }
#else
            void link_node(bucket_ptr n, bucket_ptr base)
            {
                n->next_ = base->next_;
                base->next_ = n;
                ++size_;
                if(base < cached_begin_bucket_) cached_begin_bucket_ = base;
            }

            void link_group(bucket_ptr n, bucket_ptr base, size_type)
            {
                link_node(n, base);
            }
#endif

#if BOOST_UNORDERED_HASH_EQUIVALENT
            void unlink_node(iterator_base x)
            {
                node& to_delete = get_node(x.local_.node_pointer_);
                bucket_ptr next = to_delete.next_;
                bucket_ptr* pos = get_for_erase(x);

                if(to_delete.group_prev_ == *pos) {
                    // The deleted node is the sole node in the group, so
                    // no need to unlink it from a goup.
                }
                else if(next && prev_in_group(next) == *pos)
                {
                    // The deleted node is not at the end of the group, so
                    // change the link from the next node.
                    prev_in_group(next) = to_delete.group_prev_;
                }
                else {
                    // The deleted node is at the end of the group, so the
                    // node in the group pointing to it is at the beginning
                    // of the group. Find that to change its pointer.
                    bucket_ptr it = to_delete.group_prev_;
                    while(prev_in_group(it) != *pos) {
                        it = prev_in_group(it);
                    }
                    prev_in_group(it) = to_delete.group_prev_;
                }
                *pos = (*pos)->next_;
                --size_;
            }

            size_type unlink_group(bucket_ptr* pos)
            {
                size_type count = group_count(local_iterator_base(*pos));
                size_ -= count;
                bucket_ptr last = last_in_group(*pos);
                *pos = last->next_;
                return count;
            }
#else
            void unlink_node(iterator_base x)
            {
                bucket_ptr* pos = get_for_erase(x);
                *pos = (*pos)->next_;
                --size_;
            }

            size_type unlink_group(bucket_ptr* pos)
            {
                *pos = (*pos)->next_;
                --size_;
                return 1;
            }
#endif

            void unlink_nodes(iterator_base pos)
            {
                bucket_ptr* it = get_for_erase(pos);
                split_group(*it);
                unordered_detail::reset(*it);
                size_ -= node_count(pos.local_);
            }

            void unlink_nodes(iterator_base begin, iterator_base end)
            {
                BOOST_ASSERT(begin.bucket_ == end.bucket_);
                local_iterator_base local_end = end.local_;

                size_ -= node_count(begin.local_, local_end);
                bucket_ptr* it = get_for_erase(begin);
                split_group(*it, local_end.node_pointer_);
                *it = local_end.node_pointer_;
            }

            void unlink_nodes(bucket_ptr base, iterator_base end)
            {
                BOOST_ASSERT(base == end.bucket_);

                local_iterator_base local_end = end.local_;
                split_group(local_end.node_pointer_);
            
                bucket_ptr ptr(base->next_);
                base->next_ = local_end.node_pointer_;

                size_ -= node_count(local_iterator_base(ptr), local_end);
            }

#if BOOST_UNORDERED_HASH_EQUIVALENT
            // Break a ciruclar list into two, with split as the beginneing
            // of the second group (if split is at the beginning then don't
            // split).
            bucket_ptr split_group(bucket_ptr split)
            {
                // If split is at the beginning of the group then there's
                // nothing to split.
                if(prev_in_group(split)->next_ != split)
                    return bucket_ptr();

                // Find the start of the group.
                bucket_ptr start = split;
                do {
                    start = prev_in_group(start);
                } while(prev_in_group(start)->next_ == start);

                bucket_ptr last = prev_in_group(start);
                prev_in_group(start) = prev_in_group(split);
                prev_in_group(split) = last;

                return start;
            }

            void split_group(bucket_ptr split1, bucket_ptr split2)
            {
                bucket_ptr begin1 = split_group(split1);
                bucket_ptr begin2 = split_group(split2);

                if(begin1 && split1 == begin2) {
                    bucket_ptr end1 = prev_in_group(begin1);
                    prev_in_group(begin1) = prev_in_group(begin2);
                    prev_in_group(begin2) = end1;
                }
            }
#else
            void split_group(bucket_ptr)
            {
            }

            void split_group(bucket_ptr, bucket_ptr)
            {
            }
#endif

            // throws, strong exception-safety:
            bucket_ptr construct_node(value_type const& v)
            {
                node_constructor a(allocators_);
                a.construct(v);
                return a.release();
            }

            // Create Node
            //
            // Create a node and add it to the buckets in the given position.
            //
            // strong exception safety.

            iterator_base create_node(value_type const& v, bucket_ptr base)
            {
                // throws, strong exception-safety:
                bucket_ptr n = construct_node(v);

                // Rest is no throw
                link_node(n, base);
                return iterator_base(base, n);
            }

#if BOOST_UNORDERED_HASH_EQUIVALENT
            iterator_base create_node(value_type const& v, iterator_base position)
            {
                // throws, strong exception-safety:
                bucket_ptr n = construct_node(v);

                // Rest is no throw
                link_node(n, position.local_);
                return iterator_base(position.bucket_, n);
            }

            iterator_base create_node(value_type const& v,
                    bucket_ptr base, local_iterator_base position)
            {
                // throws, strong exception-safety:
                bucket_ptr n = construct_node(v);

                // Rest is no throw
                if(position.not_finished())
                    link_node(n, position);
                else
                    link_node(n, base);

                return iterator_base(base, n);
            }
#endif

#if BOOST_UNORDERED_HASH_EQUIVALENT
            void copy_group(local_iterator_base it, bucket_ptr dst)
            {
                local_iterator_base end = it;
                end.next_group();
                iterator_base pos = create_node(*it, dst);
                for(it.increment(); it != end; it.increment())
                    create_node(*it, pos);
            }
#else
            void copy_group(local_iterator_base it, bucket_ptr dst)
            {
                create_node(*it, dst);
            }
#endif

            // Delete Node
            //
            // Remove a node, or a range of nodes, from a bucket, and destory
            // them.
            //
            // no throw

            void delete_to_bucket_end(bucket_ptr ptr)
            {
                while(ptr) {
                    bucket_ptr pos = ptr;
                    ptr = ptr->next_;
                    allocators_.destroy(pos);
                }
            }

            void delete_nodes(bucket_ptr begin, bucket_ptr end)
            {
                while(begin != end) {
                    bucket_ptr pos = begin;
                    begin = begin->next_;
                    allocators_.destroy(pos);
                }
            }

#if BOOST_UNORDERED_HASH_EQUIVALENT
            void delete_group(bucket_ptr pos)
            {
                delete_nodes(pos, prev_in_group(pos)->next_);
            }
#else
            void delete_group(bucket_ptr pos)
            {
                allocators_.destroy(pos);
            }
#endif

            // Clear
            //
            // Remove all the nodes.
            //
            // no throw

            void clear_bucket(bucket_ptr b)
            {
                bucket_ptr ptr = b->next_;
                unordered_detail::reset(b->next_);
                delete_to_bucket_end(ptr);
            }

            void clear()
            {
                bucket_ptr begin = buckets_;
                bucket_ptr end = buckets_ + bucket_count_;

                size_ = 0;
                cached_begin_bucket_ = end;

                while(begin != end) {
                    clear_bucket(begin);
                    ++begin;
                }
            }

            // Erase
            //
            // Return type of erase(const_iterator):
            // http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2005/n1753.html#130
            //
            // no throw

            iterator_base erase(iterator_base r)
            {
                BOOST_ASSERT(r != end());
                iterator_base next = r;
                next.increment();
                unlink_node(r);
                allocators_.destroy(r.local_.node_pointer_);
                // r has been invalidated but its bucket is still valid
                recompute_begin_bucket(r.bucket_, next.bucket_);
                return next;
            }

            iterator_base erase(iterator_base r1, iterator_base r2)
            {
                if(r1 != r2)
                {
                    BOOST_ASSERT(r1 != end());

                    if (r1.bucket_ == r2.bucket_) {
                        unlink_nodes(r1, r2);
                        delete_nodes(r1.local_.node_pointer_, r2.local_.node_pointer_);

                        // No need to call recompute_begin_bucket because
                        // the nodes are only deleted from one bucket, which
                        // still contains r2 after the erase.
                        BOOST_ASSERT(!r1.bucket_->empty());
                    }
                    else {
                        BOOST_ASSERT(r1.bucket_ < r2.bucket_);

                        unlink_nodes(r1);
                        delete_to_bucket_end(r1.local_.node_pointer_);

                        for(bucket_ptr i = r1.bucket_ + 1; i != r2.bucket_; ++i) {
                            size_ -= node_count(local_iterator_base(i->next_));
                            clear_bucket(i);
                        }

                        if(r2 != end()) {
                            bucket_ptr first = r2.bucket_->next_;
                            unlink_nodes(r2.bucket_, r2);
                            delete_nodes(first, r2.local_.node_pointer_);
                        }

                        // r1 has been invalidated but its bucket is still
                        // valid.
                        recompute_begin_bucket(r1.bucket_, r2.bucket_);
                    }
                }

                return r2;
            }

            // recompute_begin_bucket
            //
            // After an erase cached_begin_bucket_ might be left pointing to
            // an empty bucket, so this is called to update it
            //
            // no throw

            void recompute_begin_bucket(bucket_ptr ptr)
            {
                BOOST_ASSERT(!(ptr < cached_begin_bucket_));

                if(ptr == cached_begin_bucket_)
                {
                    if (size_ != 0) {
                        while (cached_begin_bucket_->empty())
                            ++cached_begin_bucket_;
                    } else {
                        cached_begin_bucket_ = buckets_ + bucket_count_;
                    }
                }
            }

            // This is called when a range has been erased
            //
            // no throw

            void recompute_begin_bucket(bucket_ptr i, bucket_ptr j)
            {
                BOOST_ASSERT(!(i < cached_begin_bucket_) && !(j < i));
                BOOST_ASSERT(j == buckets_ + bucket_count_ || !j->empty());

                if(i == cached_begin_bucket_ && i->empty())
                    cached_begin_bucket_ = j;
            }

            size_type erase_group(bucket_ptr* it, bucket_ptr bucket)
            {
                bucket_ptr pos = *it;
                size_type count = unlink_group(it);
                delete_group(pos);

                this->recompute_begin_bucket(bucket);

                return count;
            }
        };

#if defined(BOOST_MPL_CFG_MSVC_ETI_BUG)
        template <>
        class HASH_TABLE_DATA<int>
        {
        public:
            typedef int size_type;
            typedef int iterator_base;
        };
#endif

        //
        // Hash Table
        //

        template <class ValueType, class KeyType,
            class Hash, class Pred,
            class Alloc>
        class HASH_TABLE
            : public HASH_TABLE_DATA<Alloc>
        {
            typedef HASH_TABLE_DATA<Alloc> data;

            typedef typename data::node_constructor node_constructor;
            typedef typename data::bucket_ptr bucket_ptr;

        public:

            typedef BOOST_DEDUCED_TYPENAME data::value_allocator value_allocator;
            typedef BOOST_DEDUCED_TYPENAME data::node_allocator node_allocator;

            // Type definitions

            typedef KeyType key_type;
            typedef Hash hasher;
            typedef Pred key_equal;
            typedef ValueType value_type;
            typedef std::size_t size_type;

            // iterators

            typedef BOOST_DEDUCED_TYPENAME data::local_iterator_base local_iterator_base;
            typedef BOOST_DEDUCED_TYPENAME data::iterator_base iterator_base;

        private:

            class functions
            {
                std::pair<hasher, key_equal> functions_;

            public:

                functions(hasher const& h, key_equal const& k)
                    : functions_(h, k) {}

                hasher const& hash_function() const
                {
                    return functions_.first;
                }

                key_equal const& key_eq() const
                {
                    return functions_.second;
                }
            };

            // Both hasher and key_equal's copy/assign can throw so double
            // buffering is used to copy them. func_ points to the currently
            // active function objects.

            typedef functions HASH_TABLE::*functions_ptr;

            functions func1_;
            functions func2_;
            functions_ptr func_;

            float mlf_;
            size_type max_load_;

        public:

            // Constructors

            HASH_TABLE(size_type n,
                    hasher const& hf, key_equal const& eq,
                    value_allocator const& a)
                : data(n, a),         // throws, cleans itself up
                func1_(hf, eq),       // throws      "     "
                func2_(hf, eq),       // throws      "     "
                func_(&HASH_TABLE::func1_), // no throw
                mlf_(1.0f)            // no throw
            {
                calculate_max_load(); // no throw
            }

            // Construct from iterators

            // initial_size
            //
            // A helper function for the copy constructor to calculate how many
            // nodes will be created if the iterator's support it. Might get it
            // totally wrong for containers with unique keys.
            //
            // no throw

            template <class I>
            size_type initial_size(I i, I j, size_type n,
                    boost::forward_traversal_tag)
            {
                // max load factor isn't set yet, but when it is, it'll be 1.0.
                return (std::max)(static_cast<size_type>(std::distance(i, j)) + 1, n);
            };

            template <class I>
            size_type initial_size(I, I, size_type n,
                    boost::incrementable_traversal_tag)
            {
                return n;
            };

            template <class I>
            size_type initial_size(I i, I j, size_type x)
            {
                BOOST_DEDUCED_TYPENAME boost::iterator_traversal<I>::type
                    iterator_traversal_tag;
                return initial_size(i, j, x, iterator_traversal_tag);
            };

            template <class I>
            HASH_TABLE(I i, I j, size_type n,
                    hasher const& hf, key_equal const& eq,
                    value_allocator const& a)
                : data(initial_size(i, j, n), a),  // throws, cleans itself up
                    func1_(hf, eq),                // throws    "      "
                    func2_(hf, eq),                // throws    "      "
                    func_(&HASH_TABLE::func1_),    // no throw
                    mlf_(1.0f)                     // no throw
            {
                calculate_max_load(); // no throw

                // This can throw, but HASH_TABLE_DATA's destructor will clean up.
                insert(i, j);
            }
            // Copy Construct

            HASH_TABLE(HASH_TABLE const& x)
                : data(x, x.min_buckets_for_size(x.size())), // throws
                func1_(x.current_functions()), // throws
                func2_(x.current_functions()), // throws
                func_(&HASH_TABLE::func1_), // no throw
                mlf_(x.mlf_) // no throw
            {
                calculate_max_load(); // no throw

                // This can throw, but HASH_TABLE_DATA's destructor will clean
                // up.
                copy_buckets(x, *this, current_functions());
            }

            // Assign
            //
            // basic exception safety, if copy_functions of reserver throws
            // the container is left in a sane, empty state. If copy_buckets
            // throws the container is left with whatever was successfully
            // copied.

            HASH_TABLE& operator=(HASH_TABLE const& x)
            {
                if(this != &x)
                {
                    this->clear();                        // no throw
                    func_ = copy_functions(x);            // throws, strong
                    mlf_ = x.mlf_;                        // no throw
                    calculate_max_load();                 // no throw
                    reserve(x.size());                    // throws
                    copy_buckets(x, *this, current_functions()); // throws
                }

                return *this;
            }

            // Swap
            //
            // Swap's behaviour when allocators aren't equal is in dispute, see
            // this paper for full details:
            //
            // http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2004/n1599.html
            //
            // It lists 3 possible behaviours:
            //
            // 1 - If the allocators aren't equal then throw an exception.
            // 2 - Reallocate the elements in the containers with the
            //     appropriate allocators - messing up exception safety in
            //     the process.
            // 3 - Swap the allocators, hoping that the allocators have a
            //     no-throw swap.
            //
            // The paper recommends #3.
            //
            // I've implemented all three, but actived #3 by default, to change
            // it '#define BOOST_UNORDERED_SWAP_METHOD n' where n is the option.
            //
            // ----------------------------------------------------------------
            //
            // Strong exception safety (might change unused function objects)
            //
            // Can throw if hash or predicate object's copy constructor throws.
            // If allocators are unequal:
            //     Method 1: always throws.
            //     Method 2: can throw if copying throws
            //          (memory allocation/hash function object)
            //     Method 3: Can throw if allocator's swap throws
            //          (This one will go wrong if the allocator's swap throws
            //           but since there's no guarantee what the allocators
            //           will contain it's hard to know what to do. Maybe it
            //           could double buffer the allocators).

            void swap(HASH_TABLE& x)
            {
                // This only effects the function objects that aren't in use
                // so it is strongly exception safe, via. double buffering.
                functions_ptr new_func_this = copy_functions(x);       // throws
                functions_ptr new_func_that = x.copy_functions(*this); // throws

                if(this->allocators_.node_alloc_ == x.allocators_.node_alloc_ &&
                        this->allocators_.bucket_alloc_ == x.allocators_.bucket_alloc_) {
                    this->data::swap(x); // no throw
                }
                else {
#if BOOST_UNORDERED_SWAP_METHOD == 1
                    throw std::runtime_error(
                            "Swapping containers with different allocators.");
#elif BOOST_UNORDERED_SWAP_METHOD == 2
                    // Create new buckets in separate HASH_TABLE_DATA objects
                    // which will clean up if anything throws an exception.
                    // (all can throw, but with no effect as these are new objects).
                    data new_this(*this, x.min_buckets_for_size(x.size_));
                    copy_buckets(x, new_this, this->*new_func_this);

                    data new_that(x, min_buckets_for_size(this->size_));
                    x.copy_buckets(*this, new_that, x.*new_func_that);

                    // Start updating the data here, no throw from now on.
                    this->data::swap(new_this);
                    x.data::swap(new_that);
#elif BOOST_UNORDERED_SWAP_METHOD == 3
                    // Note: I'm not sure that these are guaranteed to be no
                    // throw.
                    hash_swap(this->allocators_.node_alloc_,
                            x.allocators_.node_alloc_);
                    hash_swap(this->allocators_.bucket_alloc_,
                            x.allocators_.bucket_alloc_);
                    this->data::swap(x);
#else
#error "Invalid swap method"
#endif
                }

                // We've made it, the rest is no throw.
                std::swap(mlf_, x.mlf_);

                func_ = new_func_this;
                x.func_ = new_func_that;

                calculate_max_load();
                x.calculate_max_load();
            }

        private:

            functions const& current_functions() const
            {
                return this->*func_;
            }

            // This copies the given function objects into the currently unused
            // function objects and returns a pointer, that func_ can later be
            // set to, to commit the change.
            //
            // Strong exception safety (since only usued function objects are
            // changed).
            functions_ptr copy_functions(HASH_TABLE const& x)
            {
                // no throw:
                functions_ptr ptr = func_ == &HASH_TABLE::func1_
                    ? &HASH_TABLE::func2_ : &HASH_TABLE::func1_;
                // throws, functions not in use, so strong
                this->*ptr = x.current_functions();
                return ptr;
            }

        public:

            // accessors

            // no throw
            value_allocator get_allocator() const
            {
                return this->allocators_.node_alloc_;
            }

            // no throw
            hasher const& hash_function() const
            {
                return current_functions().hash_function();
            }

            // no throw
            key_equal const& key_eq() const
            {
                return current_functions().key_eq();
            }

            // no throw
            size_type size() const
            {
                return this->size_;
            }

            // no throw
            bool empty() const
            {
                return this->size_ == 0;
            }

            // no throw
            size_type max_size() const
            {
                // size < mlf_ * count
                return float_to_size_t(ceil(
                        max_bucket_count() * mlf_)) - 1;
            }

            // strong safety
            size_type bucket(key_type const& k) const
            {
                // hash_function can throw:
                return this->index_from_hash(hash_function()(k));
            }

            // strong safety
            bucket_ptr get_bucket(key_type const& k) const
            {
                return this->buckets_ + bucket(k);
            }

            // no throw
            size_type bucket_count() const
            {
                return this->bucket_count_;
            }

            // no throw
            size_type max_bucket_count() const
            {
                // -1 to account for the end marker.
                return prev_prime(this->allocators_.bucket_alloc_.max_size() - 1);
            }

        private:

            // no throw
            size_type min_buckets_for_size(size_type n) const
            {
                BOOST_ASSERT(mlf_ != 0);

                using namespace std;

                // From 6.3.1/13:
                // size < mlf_ * count
                // => count > size / mlf_
                //
                // Or from rehash post-condition:
                // count > size / mlf_
                return static_cast<size_type>(floor(n / mlf_)) + 1;
            }

            // no throw
            void calculate_max_load()
            {
                using namespace std;

                // From 6.3.1/13:
                // Only resize when size >= mlf_ * count
                max_load_ = float_to_size_t(ceil(mlf_ * this->bucket_count_));
            }

            // basic exception safety
            bool reserve(size_type n)
            {
                bool need_to_reserve = n >= max_load_;
                // throws - basic:
                if (need_to_reserve) rehash_impl(min_buckets_for_size(n));
                BOOST_ASSERT(n < max_load_ || n > max_size());
                return need_to_reserve;
            }

            // basic exception safety
            //
            // This version of reserve is called when inserting a range
            // into a container with equivalent keys, it creates more buckets
            // if the resulting load factor would be over 80% of the load
            // factor. This is to try to avoid excessive rehashes.
            bool reserve_extra(size_type n)
            {
                bool need_to_reserve = n >= max_load_;
                // throws - basic:
                if (need_to_reserve) {
                    rehash_impl(static_cast<size_type>(floor(n / mlf_ * 1.25)) + 1);
                }
                BOOST_ASSERT(n < max_load_ || n > max_size());
                return need_to_reserve;
            }

        public:

            // no throw
            float max_load_factor() const
            {
                return mlf_;
            }

            // no throw
            void max_load_factor(float z)
            {
                BOOST_ASSERT(z > 0);
                mlf_ = (std::max)(z, minimum_max_load_factor);
                calculate_max_load();
            }

            // no throw
            float load_factor() const
            {
                BOOST_ASSERT(this->bucket_count_ != 0);
                return static_cast<float>(this->size_)
                    / static_cast<float>(this->bucket_count_);
            }

        private:

            // key extractors

            // no throw
            static key_type const& extract_key(value_type const& v)
            {
                return extract(v, (type_wrapper<value_type>*)0);
            }

            static key_type const& extract(value_type const& v,
                    type_wrapper<key_type>*)
            {
                return v;
            }

            static key_type const& extract(value_type const& v,
                    void*)
            {
                return v.first;
            }

        public:

            // if hash function throws, basic exception safety
            // strong otherwise.
            void rehash(size_type n)
            {
                using namespace std;

                // no throw:
                size_type min_size = min_buckets_for_size(size());
                // basic/strong:
                rehash_impl(min_size > n ? min_size : n);

                BOOST_ASSERT(bucket_count() > size() / max_load_factor()
                        && bucket_count() >= n);
            }

        private:

            // if hash function throws, basic exception safety
            // strong otherwise
            void rehash_impl(size_type n)
            {
                n = next_prime(n); // no throw

                if (n == bucket_count())  // no throw
                    return;

                data new_buckets(*this, n); // throws, seperate
                move_buckets(*this, new_buckets, hash_function());
                                                        // basic/no throw
                new_buckets.swap(*this);                // no throw
                calculate_max_load();                   // no throw
            }

            // move_buckets & copy_buckets
            //
            // if the hash function throws, basic excpetion safety
            // no throw otherwise

            static void move_buckets(data& src, data& dst, hasher const& hf)
            {
                BOOST_ASSERT(dst.size_ == 0);
                BOOST_ASSERT(src.allocators_.node_alloc_ == dst.allocators_.node_alloc_);

                bucket_ptr end = src.buckets_ + src.bucket_count_;

                for(; src.cached_begin_bucket_ != end;
                        ++src.cached_begin_bucket_) {
                    bucket_ptr src_bucket = src.cached_begin_bucket_;
                    while(src_bucket->next_) {
                        // Move the first group of equivalent nodes in
                        // src_bucket to dst.

                        // This next line throws iff the hash function throws.
                        bucket_ptr dst_bucket = dst.buckets_ +
                            dst.index_from_hash(
                                hf(extract_key(get_value(src_bucket->next_))));

                        bucket_ptr n = src_bucket->next_;
                        size_type count = src.unlink_group(&src_bucket->next_);
                        dst.link_group(n, dst_bucket, count);
                    }
                }
            }

            // basic excpetion safety. If an exception is thrown this will
            // leave dst partially filled.

            static void copy_buckets(data const& src, data& dst, functions const& f)
            {
                BOOST_ASSERT(dst.size_ == 0);
                // no throw:
                bucket_ptr end = src.buckets_ + src.bucket_count_;
                hasher const& hf = f.hash_function();

                // no throw:
                for(bucket_ptr i = src.cached_begin_bucket_; i != end; ++i) {
                    // no throw:
                    for(local_iterator_base it = src.begin(i);
                            it.not_finished(); it.next_group()) {
                        // hash function can throw.
                        bucket_ptr dst_bucket = dst.buckets_ +
                            dst.index_from_hash(hf(extract_key(*it)));
                        // throws, strong
                        dst.copy_group(it, dst_bucket);
                    }
                }
            }

        public:

            // Insert functions
            //
            // basic exception safety, if hash function throws
            // strong otherwise.

#if BOOST_UNORDERED_HASH_EQUIVALENT

            // Insert (equivalent key containers)

            // if hash function throws, basic exception safety
            // strong otherwise
            iterator_base insert(value_type const& v)
            {
                key_type const& k = extract_key(v);
                size_type hash_value = hash_function()(k);
                bucket_ptr bucket = this->buckets_
                    + this->index_from_hash(hash_value);
                local_iterator_base position = find_iterator(bucket, k);

                // Create the node before rehashing in case it throws an
                // exception (need strong safety in such a case).
                node_constructor a(this->allocators_);
                a.construct(v);

                // reserve has basic exception safety if the hash function
                // throws, strong otherwise.
                if(reserve(size() + 1))
                    bucket = this->buckets_ + this->index_from_hash(hash_value);

                // Nothing after the point can throw.

                bucket_ptr n = a.release();

                // I'm relying on local_iterator_base not being invalidated by
                // the rehash here.
                if(position.not_finished())
                    this->link_node(n, position);
                else
                    this->link_node(n, bucket);

                return iterator_base(bucket, n);
            }

            // Insert (equivalent key containers)

            // if hash function throws, basic exception safety
            // strong otherwise
            iterator_base insert(iterator_base const& it, value_type const& v)
            {
                // equal can throw, but with no effects
                if (it == this->end() || !equal(extract_key(v), *it)) {
                    // Use the standard insert if the iterator doesn't point
                    // to a matching key.
                    return insert(v);
                }
                else {
                    // Create the node before rehashing in case it throws an
                    // exception (need strong safety in such a case).
                    node_constructor a(this->allocators_);
                    a.construct(v);

                    // reserve has basic exception safety if the hash function
                    // throws, strong otherwise.
                    bucket_ptr base = reserve(size() + 1) ?
                        get_bucket(extract_key(v)) : it.bucket_;

                    // Nothing after this point can throw

                    bucket_ptr n = a.release();
                    this->link_node(n, it.local_);

                    return iterator_base(base, n);
                }
            }

            // Insert from iterator range (equivalent key containers)

        private:

            // if hash function throws, or inserting > 1 element, basic exception safety
            // strong otherwise
            template <class I>
            void insert_for_range(I i, I j, forward_traversal_tag)
            {
                size_type distance = std::distance(i, j);
                if(distance == 1) {
                    insert(*i);
                }
                else {
                    // Only require basic exception safety here
                    reserve_extra(size() + distance);

                    for (; i != j; ++i) {
                        node_constructor a(this->allocators_);
                        a.construct(*i);

                        key_type const& k = extract_key(a.get()->value_);
                        bucket_ptr bucket = get_bucket(k);
                        local_iterator_base position = find_iterator(bucket, k);

                        if(position.not_finished())
                            link_node(a.release(), position);
                        else
                            link_node(a.release(), bucket);
                    }
                }
            }

            // if hash function throws, or inserting > 1 element, basic exception safety
            // strong otherwise
            template <class I>
            void insert_for_range(I i, I j,
                    boost::incrementable_traversal_tag)
            {
                // If only inserting 1 element, get the required
                // safety since insert is only called once.
                for (; i != j; ++i) insert(*i);
            }

        public:

            // if hash function throws, or inserting > 1 element, basic exception safety
            // strong otherwise
            template <class I>
            void insert(I i, I j)
            {
                BOOST_DEDUCED_TYPENAME boost::iterator_traversal<I>::type
                    iterator_traversal_tag;
                insert_for_range(i, j, iterator_traversal_tag);
            }
#else
            // if hash function throws, basic exception safety
            // strong otherwise
            value_type& operator[](key_type const& k)
            {
                BOOST_STATIC_ASSERT((
                            !boost::is_same<value_type, key_type>::value));
                typedef BOOST_DEDUCED_TYPENAME value_type::second_type mapped_type;

                size_type hash_value = hash_function()(k);
                bucket_ptr bucket = this->buckets_ + this->index_from_hash(hash_value);
                local_iterator_base pos = find_iterator(bucket, k);

                if (pos.not_finished())
                    return *pos;
                else
                {
                    // Side effects only in this block.

                    // Create the node before rehashing in case it throws an
                    // exception (need strong safety in such a case).
                    node_constructor a(this->allocators_);
                    a.construct(value_type(k, mapped_type()));

                    // reserve has basic exception safety if the hash function
                    // throws, strong otherwise.
                    if (reserve(size() + 1))
                        bucket = this->buckets_ + this->index_from_hash(hash_value);

                    // Nothing after this point can throw.

                    bucket_ptr n = a.release();
                    this->link_node(n, bucket);

                    return *local_iterator_base(n);
                }
            }

            // Insert (unique keys)

            // if hash function throws, basic exception safety
            // strong otherwise
            std::pair<iterator_base, bool> insert(value_type const& v)
            {
                // No side effects in this initial code
                key_type const& k = extract_key(v);
                size_type hash_value = hash_function()(k);
                bucket_ptr bucket = this->buckets_ + this->index_from_hash(hash_value);
                local_iterator_base pos = find_iterator(bucket, k);
                
                if (pos.not_finished()) {
                    // Found an existing key, return it (no throw).
                    return std::pair<iterator_base, bool>(
                        iterator_base(bucket, pos), false);

                } else {
                    // Doesn't already exist, add to bucket.
                    // Side effects only in this block.

                    // Create the node before rehashing in case it throws an
                    // exception (need strong safety in such a case).
                    node_constructor a(this->allocators_);
                    a.construct(v);

                    // reserve has basic exception safety if the hash function
                    // throws, strong otherwise.
                    if(reserve(size() + 1))
                        bucket = this->buckets_ + this->index_from_hash(hash_value);

                    // Nothing after this point can throw.

                    bucket_ptr n = a.release();
                    this->link_node(n, bucket);

                    return std::pair<iterator_base, bool>(
                        iterator_base(bucket, n), true);
                }
            }

            // Insert (unique keys)

            // if hash function throws, basic exception safety
            // strong otherwise
            iterator_base insert(iterator_base const& it, value_type const& v)
            {
                if(it != this->end() && equal(extract_key(v), *it))
                    return it;
                else
                    return insert(v).first;
            }

            // Insert from iterators (unique keys)

            template <class I>
            size_type insert_size(I i, I j, boost::forward_traversal_tag)
            {
                return std::distance(i, j);
            }

            template <class I>
            size_type insert_size(I i, I j, boost::incrementable_traversal_tag)
            {
                return 1;
            }

            template <class I>
            size_type insert_size(I i, I j)
            {
                BOOST_DEDUCED_TYPENAME boost::iterator_traversal<I>::type
                    iterator_traversal_tag;
                return insert_size(i, j, iterator_traversal_tag);
            };

            // if hash function throws, or inserting > 1 element, basic exception safety
            // strong otherwise
            template <class InputIterator>
            void insert(InputIterator i, InputIterator j)
            {
                // If only inserting 1 element, get the required
                // safety since insert is only called once.
                for (; i != j; ++i) {
                    // No side effects in this initial code
                    size_type hash_value = hash_function()(extract_key(*i));
                    bucket_ptr bucket = this->buckets_
                        + this->index_from_hash(hash_value);
                    local_iterator_base pos = find_iterator(bucket, extract_key(*i));
                    
                    if (!pos.not_finished()) {
                        // Doesn't already exist, add to bucket.
                        // Side effects only in this block.

                        // Create the node before rehashing in case it throws an
                        // exception (need strong safety in such a case).
                        node_constructor a(this->allocators_);
                        value_type const& v = *i;
                        a.construct(v);

                        // reserve has basic exception safety if the hash function
                        // throws, strong otherwise.
                        if(size() + 1 >= max_load_) {
                            reserve(size() + insert_size(i, j));
                            bucket = this->buckets_ + this->index_from_hash(hash_value);
                        }

                        // Nothing after this point can throw.
                        this->link_node(a.release(), bucket);
                    }
                }
            }
#endif
        public:

            // erase

            // no throw
            iterator_base erase(iterator_base const& r)
            {
                return this->data::erase(r);
            }

            // strong exception safety
            size_type erase(key_type const& k)
            {
                // No side effects in initial section
                bucket_ptr bucket = get_bucket(k);
                bucket_ptr* it = find_for_erase(bucket, k);

                // No throw.
                return *it ? this->erase_group(it, bucket) : 0;
            }

            // no throw
            iterator_base erase(iterator_base const& r1, iterator_base const& r2)
            {
                return this->data::erase(r1, r2);
            }

            // count
            //
            // strong exception safety, no side effects
            size_type count(key_type const& k) const
            {
                local_iterator_base it = find_iterator(k); // throws, strong
                return it.not_finished() ? group_count(it) : 0;
            }

            // find
            //
            // strong exception safety, no side effects
            iterator_base find(key_type const& k) const
            {
                bucket_ptr bucket = get_bucket(k);
                local_iterator_base it = find_iterator(bucket, k);

                if (it.not_finished())
                    return iterator_base(bucket, it);
                else
                    return this->end();
            }

            // equal_range
            //
            // strong exception safety, no side effects
            std::pair<iterator_base, iterator_base> equal_range(key_type const& k) const
            {
                bucket_ptr bucket = get_bucket(k);
                local_iterator_base it = find_iterator(bucket, k);
                if (it.not_finished()) {
                    iterator_base first(iterator_base(bucket, it));
                    iterator_base second(iterator_base(bucket, last_in_group(it.node_pointer_)));
                    second.increment();
                    return std::pair<iterator_base, iterator_base>(first, second);
                }
                else {
                    return std::pair<iterator_base, iterator_base>(
                            this->end(), this->end());
                }
            }

        private:

            // strong exception safety, no side effects
            bool equal(key_type const& k, value_type const& v) const
            {
                return key_eq()(k, extract_key(v));
            }

            // strong exception safety, no side effects
            local_iterator_base find_iterator(key_type const& k) const
            {
                return find_iterator(get_bucket(k), k);
            }

            // strong exception safety, no side effects
            local_iterator_base find_iterator(bucket_ptr bucket,
                    key_type const& k) const
            {
                local_iterator_base it = this->begin(bucket);
                while (it.not_finished() && !equal(k, *it))
                    it.next_group();

                return it;
            }

            // strong exception safety, no side effects
            bucket_ptr* find_for_erase(bucket_ptr bucket, key_type const& k) const
            {
                bucket_ptr* it = &bucket->next_;
                while(*it && !equal(k, this->get_value(*it)))
                    it = &this->next_group(*it);

                return it;
            }
        };

        // Iterators
        
        template <class Alloc> class HASH_ITERATOR;
        template <class Alloc> class HASH_CONST_ITERATOR;
        template <class Alloc> class HASH_LOCAL_ITERATOR;
        template <class Alloc> class HASH_CONST_LOCAL_ITERATOR;
        class iterator_access;

        // Local Iterators
        //
        // all no throw

        template <class Alloc>
        class HASH_LOCAL_ITERATOR
            : public boost::iterator <
                std::forward_iterator_tag,
                BOOST_DEDUCED_TYPENAME allocator_value_type<Alloc>::type,
                std::ptrdiff_t,
                BOOST_DEDUCED_TYPENAME allocator_pointer<Alloc>::type,
                BOOST_DEDUCED_TYPENAME allocator_reference<Alloc>::type >
        {
        public:
            typedef BOOST_DEDUCED_TYPENAME allocator_value_type<Alloc>::type value_type;

        private:
            typedef BOOST_DEDUCED_TYPENAME HASH_TABLE_DATA<Alloc>::local_iterator_base base;
            typedef HASH_CONST_LOCAL_ITERATOR<Alloc> const_local_iterator;

            friend class HASH_CONST_LOCAL_ITERATOR<Alloc>;
            base base_;

        public:
            HASH_LOCAL_ITERATOR() : base_() {}
            explicit HASH_LOCAL_ITERATOR(base x) : base_(x) {}
            BOOST_DEDUCED_TYPENAME allocator_reference<Alloc>::type operator*() const
                { return *base_; }
            value_type* operator->() const { return &*base_; }
            HASH_LOCAL_ITERATOR& operator++() { base_.increment(); return *this; }
            HASH_LOCAL_ITERATOR operator++(int) { HASH_LOCAL_ITERATOR tmp(base_); base_.increment(); return tmp; }
            bool operator==(HASH_LOCAL_ITERATOR x) const { return base_ == x.base_; }
            bool operator==(const_local_iterator x) const { return base_ == x.base_; }
            bool operator!=(HASH_LOCAL_ITERATOR x) const { return base_ != x.base_; }
            bool operator!=(const_local_iterator x) const { return base_ != x.base_; }
        };

        template <class Alloc>
        class HASH_CONST_LOCAL_ITERATOR
            : public boost::iterator <
                std::forward_iterator_tag,
                BOOST_DEDUCED_TYPENAME allocator_value_type<Alloc>::type,
                std::ptrdiff_t,
                BOOST_DEDUCED_TYPENAME allocator_const_pointer<Alloc>::type,
                BOOST_DEDUCED_TYPENAME allocator_const_reference<Alloc>::type >
        {
        public:
            typedef BOOST_DEDUCED_TYPENAME allocator_value_type<Alloc>::type value_type;

        private:
            typedef BOOST_DEDUCED_TYPENAME HASH_TABLE_DATA<Alloc>::local_iterator_base base;
            typedef HASH_LOCAL_ITERATOR<Alloc> local_iterator;
            friend class HASH_LOCAL_ITERATOR<Alloc>;
            base base_;

        public:
            HASH_CONST_LOCAL_ITERATOR() : base_() {}
            explicit HASH_CONST_LOCAL_ITERATOR(base x) : base_(x) {}
            HASH_CONST_LOCAL_ITERATOR(local_iterator x) : base_(x.base_) {}
            BOOST_DEDUCED_TYPENAME allocator_const_reference<Alloc>::type
                operator*() const { return *base_; }
            value_type const* operator->() const { return &*base_; }
            HASH_CONST_LOCAL_ITERATOR& operator++() { base_.increment(); return *this; }
            HASH_CONST_LOCAL_ITERATOR operator++(int) { HASH_CONST_LOCAL_ITERATOR tmp(base_); base_.increment(); return tmp; }
            bool operator==(local_iterator x) const { return base_ == x.base_; }
            bool operator==(HASH_CONST_LOCAL_ITERATOR x) const { return base_ == x.base_; }
            bool operator!=(local_iterator x) const { return base_ != x.base_; }
            bool operator!=(HASH_CONST_LOCAL_ITERATOR x) const { return base_ != x.base_; }
        };

        // iterators
        //
        // all no throw


        template <class Alloc>
        class HASH_ITERATOR
            : public boost::iterator <
                std::forward_iterator_tag,
                BOOST_DEDUCED_TYPENAME allocator_value_type<Alloc>::type,
                std::ptrdiff_t,
                BOOST_DEDUCED_TYPENAME allocator_pointer<Alloc>::type,
                BOOST_DEDUCED_TYPENAME allocator_reference<Alloc>::type >
        {
        public:
            typedef BOOST_DEDUCED_TYPENAME allocator_value_type<Alloc>::type value_type;

        private:
            typedef BOOST_DEDUCED_TYPENAME HASH_TABLE_DATA<Alloc>::iterator_base base;
            typedef HASH_CONST_ITERATOR<Alloc> const_iterator;
            friend class HASH_CONST_ITERATOR<Alloc>;
            base base_;

        public:

            HASH_ITERATOR() : base_() {}
            explicit HASH_ITERATOR(base const& x) : base_(x) {}
            BOOST_DEDUCED_TYPENAME allocator_reference<Alloc>::type
                operator*() const { return *base_; }
            value_type* operator->() const { return &*base_; }
            HASH_ITERATOR& operator++() { base_.increment(); return *this; }
            HASH_ITERATOR operator++(int) { HASH_ITERATOR tmp(base_); base_.increment(); return tmp; }
            bool operator==(HASH_ITERATOR const& x) const { return base_ == x.base_; }
            bool operator==(const_iterator const& x) const { return base_ == x.base_; }
            bool operator!=(HASH_ITERATOR const& x) const { return base_ != x.base_; }
            bool operator!=(const_iterator const& x) const { return base_ != x.base_; }
        };

        template <class Alloc>
        class HASH_CONST_ITERATOR
            : public boost::iterator <
                std::forward_iterator_tag,
                BOOST_DEDUCED_TYPENAME allocator_value_type<Alloc>::type,
                std::ptrdiff_t,
                BOOST_DEDUCED_TYPENAME allocator_const_pointer<Alloc>::type,
                BOOST_DEDUCED_TYPENAME allocator_const_reference<Alloc>::type >
        {
        public:
            typedef BOOST_DEDUCED_TYPENAME allocator_value_type<Alloc>::type value_type;

        private:
            typedef BOOST_DEDUCED_TYPENAME HASH_TABLE_DATA<Alloc>::iterator_base base;
            typedef HASH_ITERATOR<Alloc> iterator;
            friend class HASH_ITERATOR<Alloc>;
            friend class iterator_access;
            base base_;

        public:

            HASH_CONST_ITERATOR() : base_() {}
            explicit HASH_CONST_ITERATOR(base const& x) : base_(x) {}
            HASH_CONST_ITERATOR(iterator const& x) : base_(x.base_) {}
            BOOST_DEDUCED_TYPENAME allocator_const_reference<Alloc>::type
                operator*() const { return *base_; }
            value_type const* operator->() const { return &*base_; }
            HASH_CONST_ITERATOR& operator++() { base_.increment(); return *this; }
            HASH_CONST_ITERATOR operator++(int) { HASH_CONST_ITERATOR tmp(base_); base_.increment(); return tmp; }
            bool operator==(iterator const& x) const { return base_ == x.base_; }
            bool operator==(HASH_CONST_ITERATOR const& x) const { return base_ == x.base_; }
            bool operator!=(iterator const& x) const { return base_ != x.base_; }
            bool operator!=(HASH_CONST_ITERATOR const& x) const { return base_ != x.base_; }
        };
    }
}

#undef HASH_TABLE
#undef HASH_TABLE_DATA
#undef HASH_ITERATOR
#undef HASH_CONST_ITERATOR
#undef HASH_LOCAL_ITERATOR
#undef HASH_CONST_LOCAL_ITERATOR


