#ifndef RBTREE_H_
#define RBTREE_H_

#include <cstdint>
#include <iterator>
#include <algorithm>

template <class T> class RBTree;

namespace RBTreeBase_ {

struct Node_ {
  Node_ *left, *right, *parent;
  size_t size;
  uint8_t black_height; // 1 byte is very sufficient
  bool black;
  Node_() : left(nullptr), right(nullptr), parent(nullptr),
    size(1), black_height(0), black(false) {}
  Node_(const Node_& x, Node_* p) : left(nullptr), right(nullptr), parent(p),
    size(x.size), black_height(x.black_height), black(x.black) {}
};

template <class T> struct NodeVal_ : Node_ {
  T value;
  NodeVal_(const T& val) : Node_(), value(val) {}
  NodeVal_(T&& val) : Node_(), value(std::move(val)) {}
  template <class... Args> NodeVal_(Args&&... args) : Node_(), value(args...) {}
  NodeVal_(const NodeVal_& x, Node_* p) : Node_(x, p), value(x.value) {}
};

inline Node_* First_(Node_* nd) {
  for (; nd->left; nd = nd->left);
  return nd;
}
inline Node_* Last_(Node_* nd) {
  for (; nd->right; nd = nd->right);
  return nd;
}

inline void ConnectLeft_(Node_* p, Node_* ch) {
  p->left = ch;
  if (ch) ch->parent = p;
}
inline void ConnectLeftNoCheck_(Node_* p, Node_* ch) {
  p->left = ch; ch->parent = p;
}
inline void ConnectRight_(Node_* p, Node_* ch) {
  p->right = ch;
  if (ch) ch->parent = p;
}
inline void ConnectRightNoCheck_(Node_* p, Node_* ch) {
  p->right = ch; ch->parent = p;
}
inline void ConnectParent_(Node_* orig, Node_* n) {
  Node_* p = n->parent = orig->parent;
  if (p) (p->left == orig ? p->left : p->right) = n;
}
inline void ConnectParentNoCheck_(Node_* orig, Node_* n) {
  Node_* p = n->parent = orig->parent;
  (p->left == orig ? p->left : p->right) = n;
}
inline size_t Size_(Node_* nd) {
  return nd ? nd->size : 0;
}

inline Node_* Next_(Node_* nd) {
  if (nd->right) return First_(nd->right);
  for (; nd->parent && nd->parent->right == nd; nd = nd->parent);
  return nd->parent;
}
inline Node_* Prev_(Node_* nd) {
  if (nd->left) return Last_(nd->left);
  for (; nd->parent && nd->parent->left == nd; nd = nd->parent);
  return nd->parent;
}
inline Node_* Select_(Node_* nd, size_t x) {
  while (true) {
    if (Size_(nd->left) == x) return nd;
    if (Size_(nd->left) > x) {
      nd = nd->left;
    } else {
      x -= Size_(nd->left) + 1;
      nd = nd->right;
    }
  }
}
inline Node_* Advance_(Node_* nd, ptrdiff_t x) {
  if (!x) return nd;
  if (x < 0) {
    size_t g = -x;
    while (Size_(nd->left) < g) {
      g -= Size_(nd->left) + 1;
      for (; nd->parent && nd->parent->left == nd; nd = nd->parent);
      nd = nd->parent;
      if (!g) return nd;
    }
    return Select_(nd->left, Size_(nd->left) - g);
  } else {
    size_t g = x;
    while (Size_(nd->right) < g) {
      g -= Size_(nd->right) + 1;
      for (; nd->parent && nd->parent->right == nd; nd = nd->parent);
      nd = nd->parent;
      if (!g) return nd;
    }
    return Select_(nd->right, g - 1);
  }
}

inline size_t Order_(Node_* nd) {
  size_t ans = Size_(nd->left);
  for (; nd->parent; nd = nd->parent) {
    if (nd->parent->right == nd) ans += Size_(nd->parent->left) + 1;
  }
  return ans;
}
inline ptrdiff_t Difference_(Node_* a, Node_* b) {
  return (ptrdiff_t)Order_(a) - (ptrdiff_t)Order_(b);
}

template <class T> class ConstIterator_;
template <class T> class Iterator_ {
  Node_* ptr_;
  typedef Iterator_ Self_;
  Iterator_(Node_* ptr) : ptr_(ptr) {}
  Iterator_(NodeVal_<T>* ptr) : ptr_(static_cast<Node_*>(ptr)) {}
 public:
  // iterator tags
  typedef T value_type;
  typedef T& reference;
  typedef T* pointer;
  typedef std::random_access_iterator_tag iterator_category;
  typedef ptrdiff_t difference_type;

  Iterator_() : ptr_(nullptr) {}

  reference operator*() const { return static_cast<NodeVal_<T>*>(ptr_)->value; }
  pointer operator->() const { return &static_cast<NodeVal_<T>*>(ptr_)->value; }
  Self_& operator++() { ptr_ = Next_(ptr_); return *this; }
  Self_& operator--() { ptr_ = Prev_(ptr_); return *this; }
  Self_ operator++(int) {
    Self_ tmp = *this;
    ptr_ = Next_(ptr_);
    return tmp;
  }
  Self_ operator--(int) {
    Self_ tmp = *this;
    ptr_ = Prev_(ptr_);
    return tmp;
  }
  Self_& operator+=(difference_type x) {
    ptr_ = Advance_(ptr_, x);
    return *this;
  }
  Self_& operator-=(difference_type x) {
    ptr_ = Advance_(ptr_, -x);
    return *this;
  }
  Self_ operator+(difference_type x) const { return Advance_(ptr_, x); }
  Self_ operator-(difference_type x) const { return Advance_(ptr_, -x); }
  difference_type operator-(const Self_& it) const {
    return Difference_(ptr_, it.ptr_);
  }
  reference operator[](difference_type x) const {
    return static_cast<NodeVal_<T>*>(Advance_(ptr_, x))->value;
  }
  bool operator==(const Self_& it) const { return ptr_ == it.ptr_; }
  bool operator!=(const Self_& it) const { return ptr_ != it.ptr_; }
  bool operator<(const Self_& it) const {
    return Difference_(ptr_, it.ptr_) < 0;
  }
  bool operator<=(const Self_& it) const { return !(it < *this); }
  bool operator>(const Self_& it) const { return it < *this; }
  bool operator>=(const Self_& it) const { return !(*this < it); }

  bool is_null() const { return !ptr_; }
  operator bool() const { return !ptr_; }
  size_t tree_size() const { return ptr_->size; }
  Self_ left_child() const { return ptr_->left; }
  Self_ right_child() const { return ptr_->right; }

  friend class ConstIterator_<T>;
  friend class RBTree<T>;
};

template <class T>
inline Iterator_<T> operator+(ptrdiff_t x, Iterator_<T> it) {
  return it + x;
}

template <class T> class ConstIterator_ {
  Node_* ptr_;
  typedef ConstIterator_ Self_;

  ConstIterator_(Node_* ptr) : ptr_(ptr) {}
  ConstIterator_(NodeVal_<T>* ptr) : ptr_(static_cast<Node_*>(ptr)) {}
 public:
  // iterator tags
  typedef T value_type;
  typedef const T& reference;
  typedef const T* pointer;
  typedef std::random_access_iterator_tag iterator_category;
  typedef ptrdiff_t difference_type;

  ConstIterator_() : ptr_(nullptr) {}
  ConstIterator_(const Iterator_<T>& it) : ptr_(it.ptr_) {}

  reference operator*() const { return static_cast<NodeVal_<T>*>(ptr_)->value; }
  pointer operator->() const { return &static_cast<NodeVal_<T>*>(ptr_)->value; }
  Self_& operator++() { ptr_ = Next_(ptr_); return *this; }
  Self_& operator--() { ptr_ = Prev_(ptr_); return *this; }
  Self_ operator++(int) {
    Self_ tmp = *this;
    ptr_ = Next_(ptr_);
    return tmp;
  }
  Self_ operator--(int) {
    Self_ tmp = *this;
    ptr_ = Prev_(ptr_);
    return tmp;
  }
  Self_& operator+=(difference_type x) {
    ptr_ = Advance_(ptr_, x);
    return *this;
  }
  Self_& operator-=(difference_type x) {
    ptr_ = Advance_(ptr_, -x);
    return *this;
  }
  Self_ operator+(difference_type x) const { return Advance_(ptr_, x); }
  Self_ operator-(difference_type x) const { return Advance_(ptr_, -x); }
  difference_type operator-(Self_ it) const {
    return Difference_(ptr_, it.ptr_);
  }
  reference operator[](difference_type x) const {
    return static_cast<NodeVal_<T>*>(Advance_(ptr_, x))->value;
  }
  bool operator==(const Self_& it) const { return ptr_ == it.ptr_; }
  bool operator!=(const Self_& it) const { return ptr_ != it.ptr_; }
  bool operator<(const Self_& it) const {
    return Difference_(ptr_, it.ptr_) < 0;
  }
  bool operator<=(const Self_& it) const { return !(it < *this); }
  bool operator>(const Self_& it) const { return it < *this; }
  bool operator>=(const Self_& it) const { return !(*this < it); }

  bool is_null() const { return !ptr_; }
  operator bool() const { return !ptr_; }
  size_t tree_size() const { return ptr_->size; }
  Self_ left_child() const { return ptr_->left; }
  Self_ right_child() const { return ptr_->right; }

  friend class RBTree<T>;
};

template <class T>
inline ConstIterator_<T> operator+(ptrdiff_t x, ConstIterator_<T> it) {
  return it + x;
}

} // namespace RBTreeBase_

#ifdef DEBUG
#include <cstdio>
bool MySwitch = false;
#endif

template <class T> class RBTree {
  typedef RBTreeBase_::Node_ NodeBase_;
  typedef RBTreeBase_::NodeVal_<T> NodeType_;

#ifdef DEBUG
  void Print_(NodeType_* a) const {
    if (!a) {
      printf("x ");
      return;
    }
    printf("%d,sz%d,bh%d,%c ", a->value, (int)a->size, (int)a->black_height, a->black ? 'B' : 'R');
    if (a->left || a->right) {
      printf("(");
      Print_((NodeType_*)a->left);
      Print_((NodeType_*)a->right);
      printf(") ");
    }
  }

  bool CheckRB_(NodeBase_* a) const {
    if (!a) return true;
    uint8_t z = a->black_height - a->black;
    auto H = [](NodeBase_* a){ return a ? a->black_height : 0; };
    if (H(a->left) != z || H(a->right) != z || (!a->black &&
         ((a->left && !a->left->black) || (a->right && !a->right->black))) ||
        (a->left && a->left->parent != a) ||
        (a->right && a->right->parent != a) ||
        a->size != Size_(a->left) + Size_(a->right) + 1) return false;
    return CheckRB_(a->left) && CheckRB_(a->right);
  }
  bool CheckRB_() const {
    return !head_->left || (head_->left->black && CheckRB_(head_->left));
  }
#endif

  virtual void Pull_(NodeBase_* nd) const {}

  NodeBase_* Base_(NodeType_* nd) const {
    return static_cast<NodeBase_*>(nd);
  }
  NodeType_* Sup_(NodeBase_* nd) const {
    return static_cast<NodeType_*>(nd);
  }

  void PullSize_(NodeBase_* nd) const {
    nd->size = Size_(nd->left) + Size_(nd->right) + 1;
    Pull_(nd);
  }
  void PullSizeNoCheck_(NodeBase_* nd) const {
    nd->size = nd->left->size + nd->right->size + 1;
    Pull_(nd);
  }
  NodeBase_* IncreaseSize_(NodeBase_* nd, NodeBase_* head, size_t sz = 1) const {
    while (true) {
      nd->size += sz; Pull_(nd);
      if (nd->parent == head) return nd;
      nd = nd->parent;
    }
  }
  void DecreaseSize_(NodeBase_* nd) const {
    for (; nd != head_; nd = nd->parent) nd->size--, Pull_(nd);
  }
  void PaintBlack_(NodeBase_* nd) const {
    if (nd) nd->black_height += !nd->black, nd->black = true;
  }

  NodeBase_* InsertRepair_(NodeBase_* nd, NodeBase_* head, size_t sz = 1) {
    Pull_(nd);
    while (true) {
      NodeBase_* p = nd->parent;
      if (p == head) { // Case 1
        nd->black = true;
        nd->black_height++;
        return nd;
      }
      if (p->black) { // Case 2
        return IncreaseSize_(p, head, sz);
      }
      NodeBase_* g = p->parent;
      NodeBase_* u = g->left == p ? g->right : g->left;
      if (MySwitch) printf("%d\n", Sup_(g)->value);
      if (!u || u->black) { // Case 4
        if (p == g->left) {
          if (nd == p->right) {
            std::swap(nd, p);
            ConnectRight_(nd, p->left);
            ConnectLeftNoCheck_(p, nd);
            PullSize_(nd);
          }
          ConnectParent_(g, p);
          ConnectLeft_(g, p->right);
          ConnectRightNoCheck_(p, g);
        } else {
          if (nd == p->left) {
            std::swap(nd, p);
            ConnectLeft_(nd, p->right);
            ConnectRightNoCheck_(p, nd);
            PullSize_(nd);
          }
          ConnectParent_(g, p);
          ConnectRight_(g, p->left);
          ConnectLeftNoCheck_(p, g);
        }
        PullSize_(g);
        g->black = false; g->black_height--;
        PullSizeNoCheck_(p);
        p->black = true; p->black_height++;
        if (p->parent == head) return p;
        return IncreaseSize_(p->parent, head, sz);
      }
      // Case 3
      p->size += sz; p->black = true; p->black_height++; Pull_(p);
      g->size += sz; g->black = false; Pull_(g);
      u->black = true; u->black_height++;
      nd = g;
    }
  }

  void RemoveRepair_(NodeBase_* p, NodeBase_* s) {
    if (p == head_) return;
    NodeBase_* nd = nullptr;
    while (true) {
      if (!s->black) { // Case 2
        p->black = false; p->black_height--;
        s->black = true; s->black_height++;
        ConnectParentNoCheck_(p, s);
        if (p->left == s) {
          ConnectLeft_(p, s->right);
          ConnectRightNoCheck_(s, p);
          PullSize_(p); PullSize_(s);
          p->size++, s->size++;
          s = p->left;
        } else {
          ConnectRight_(p, s->left);
          ConnectLeftNoCheck_(s, p);
          PullSize_(p); PullSize_(s);
          p->size++, s->size++;
          s = p->right;
        }
        break;
      }
      if (p->black && (!s->left || s->left->black) &&
          (!s->right || s->right->black)) { // Case 3
        s->black = false; s->black_height--;
        p->size--; p->black_height--; Pull_(p);
        nd = p; p = nd->parent;
        if (p == head_) return; // Case 1
        s = p->left == nd ? p->right : p->left;
        continue;
      }
      break;
    }
    // s is black here
    NodeBase_* sin = p->left == s ? s->right : s->left;
    NodeBase_* sout = p->left == s ? s->left : s->right;
    if (sout && !sout->black) { // Case 6
      sout->black = true; sout->black_height++;
      s->black_height += p->black;
      p->black_height -= p->black;
      s->black = p->black; p->black = true;
      ConnectParentNoCheck_(p, s);
      if (p->left == s) {
        ConnectLeft_(p, s->right);
        ConnectRightNoCheck_(s, p);
      } else {
        ConnectRight_(p, s->left);
        ConnectLeftNoCheck_(s, p);
      }
      PullSize_(p); PullSizeNoCheck_(s);
      DecreaseSize_(s->parent);
    } else if (sin && !sin->black) { // Case 5
      p->black_height -= p->black;
      sin->black_height += 1 + p->black;
      sin->black = p->black;
      p->black = true;
      ConnectParentNoCheck_(p, sin);
      if (p->left == s) {
        ConnectRight_(s, sin->left);
        ConnectLeft_(p, sin->right);
        ConnectRightNoCheck_(sin, p);
        ConnectLeftNoCheck_(sin, s);
      } else {
        ConnectLeft_(s, sin->right);
        ConnectRight_(p, sin->left);
        ConnectLeftNoCheck_(sin, p);
        ConnectRightNoCheck_(sin, s);
      }
      PullSize_(p); PullSize_(s); PullSizeNoCheck_(sin);
      DecreaseSize_(sin->parent);
    } else { // Case 4, p is red here (or it will be Case 3)
      s->black = false; s->black_height--;
      p->black = true;
      DecreaseSize_(p);
    }
  }

  void InsertBefore_(NodeBase_* a, NodeType_* b) {
    if (a != head_) {
      if (!a->left) {
        ConnectLeftNoCheck_(a, b);
      } else {
        ConnectRightNoCheck_(Last_(a->left), b);
      }
    } else if (!head_->left) {
      ConnectLeftNoCheck_(head_, b);
    } else {
      ConnectRightNoCheck_(Last_(head_->left), b);
    }
    InsertRepair_(Base_(b), head_);
  }
  NodeBase_* Remove_(NodeBase_* a) {
    if (a->left && a->right) {
      NodeBase_* tmp = Last_(a->left);
      using std::swap; swap(Sup_(tmp)->value, Sup_(a)->value);
      a = tmp;
    }
    if (!a->black) { // no child
      (a->parent->left == a ? a->parent->left : a->parent->right) = nullptr;
      DecreaseSize_(a->parent);
    } else {
      NodeBase_* child = a->left ? a->left : a->right;
      if (child) { // child must be red
        child->black = true; child->black_height++;
        ConnectParent_(a, child);
        DecreaseSize_(child->parent);
      } else if (a->parent->left == a) { // no child
        a->parent->left = nullptr;
        RemoveRepair_(a->parent, a->parent->right);
      } else {
        a->parent->right = nullptr;
        RemoveRepair_(a->parent, a->parent->left);
      }
    }
    return a;
  }
  NodeBase_* Merge_(NodeBase_* l, NodeBase_* m, NodeBase_* r) {
    if (!l) {
      m->left = m->right = nullptr; m->size = 1;
      if (!r) {
        m->black = true; m->black_height = 1;
        return m;
      }
      m->black = false; m->black_height = 0;
      ConnectLeftNoCheck_(First_(r), m);
      return InsertRepair_(m, r->parent = nullptr);
    }
    if (!r) {
      m->left = m->right = nullptr; m->size = 1;
      m->black = false; m->black_height = 0;
      ConnectRightNoCheck_(Last_(l), m);
      return InsertRepair_(m, l->parent = nullptr);
    }
    if (l->black_height == r->black_height){
      ConnectLeftNoCheck_(m, l);
      ConnectRightNoCheck_(m, r);
      PullSizeNoCheck_(m);
      m->black = true; m->black_height = l->black_height + 1;
      return m;
    }
    if (l->black_height < r->black_height) {
      NodeBase_* ret = r;
      for (; !r->black || l->black_height != r->black_height; r = r->left);
      ConnectParentNoCheck_(r, m);
      ConnectLeftNoCheck_(m, l);
      ConnectRightNoCheck_(m, r);
      PullSizeNoCheck_(m);
      m->black = false; m->black_height = l->black_height;
      ret = InsertRepair_(m, ret->parent = nullptr, l->size + 1);
      return ret;
    } else {
      NodeBase_* ret = l;
      for (; !l->black || l->black_height != r->black_height; l = l->right);
      ConnectParentNoCheck_(l, m);
      ConnectLeftNoCheck_(m, l);
      ConnectRightNoCheck_(m, r);
      PullSizeNoCheck_(m);
      m->black = false; m->black_height = l->black_height;
      ret = InsertRepair_(m, ret->parent = nullptr, r->size + 1);
      return ret;
    }
  }
  void Split_(NodeBase_* nd, NodeBase_*& left, NodeBase_*& right, bool pivot) {
    NodeBase_* p = nd->parent;
    left = nd->left; right = nd->right;
    PaintBlack_(left); PaintBlack_(right);
    if (pivot) right = Merge_(nullptr, nd, right);
    while (p != head_) {
      bool is_left = p->left == nd;
      nd = p;
      p = p->parent;
      if (is_left) {
        PaintBlack_(nd->right);
        right = Merge_(right, nd, nd->right);
      } else {
        PaintBlack_(nd->left);
        left = Merge_(nd->left, nd, left);
      }
    }
  }

  template <class Pred> NodeBase_* PartitionBound_(Pred func) {
    // first element x that func(x) is false, assuming monotonicity
    NodeBase_ *now = head_->left, *last = head_;
    while (now) {
      const T& val = Sup_(now)->value; // just add constness
      if (func(val)) {
        now = now->right;
      } else {
        last = now;
        now = now->left;
      }
    }
    return last;
  }
  template <class Pred> NodeBase_* PartitionBoundIter_(Pred func) {
    // same as PartitionBound, but const_iterator is passed to func
    NodeBase_ *now = head_->left, *last = head_;
    while (now) {
      if (func(const_iterator(now))) {
        now = now->right;
      } else {
        last = now;
        now = now->left;
      }
    }
    return last;
  }

  NodeType_* GenNode_(const T& val) const {
    NodeType_* ptr = static_cast<NodeType_*>(malloc(sizeof(NodeType_)));
    new(ptr) NodeType_(val);
    return ptr;
  }
  NodeType_* GenNode_(T&& val) const {
    NodeType_* ptr = static_cast<NodeType_*>(malloc(sizeof(NodeType_)));
    new(ptr) NodeType_(std::move(val));
    return ptr;
  }
  NodeType_* GenNode_(NodeType_* x, NodeBase_* p) const {
    NodeType_* ptr = static_cast<NodeType_*>(malloc(sizeof(NodeType_)));
    new(ptr) NodeType_(*x, p);
    return ptr;
  }
  template <class... Args> NodeType_* GenNodeArgs_(Args&&... args) const {
    NodeType_* ptr = static_cast<NodeType_*>(malloc(sizeof(NodeType_)));
    new(ptr) NodeType_(args...);
    return ptr;
  }
  void FreeNode_(NodeBase_* nd) const {
    Sup_(nd)->value.~T();
    free(nd);
  }

  void ClearTree_(NodeBase_* nd) {
    NodeBase_* now = nd;
    while (true) {
      NodeBase_* tmp = now;
      if (now->left) {
        now = now->left;
        tmp->left = nullptr;
      } else if (now->right) {
        now = now->right;
        tmp->right = nullptr;
      } else if (now == nd) {
        break;
      } else {
        now = now->parent;
        FreeNode_(tmp);
      }
    }
  }
  void ClearTree_() { ClearTree_(head_); }

  void CopyTree_(NodeBase_* dest, NodeBase_* orig) {
    NodeBase_* now = orig;
    while (true) {
      if (!dest->left && now->left) {
        dest->left = Base_(GenNode_(Sup_(now->left), dest));
        dest = dest->left;
        now = now->left;
      } else if (!dest->right && now->right) {
        dest->right = Base_(GenNode_(Sup_(now->right), dest));
        dest = dest->right;
        now = now->right;
      } else if (now == orig) {
        break;
      } else {
        dest = dest->parent;
        now = now->parent;
      }
    }
  }

  void Init_() {
    head_ = static_cast<NodeBase_*>(malloc(sizeof(NodeBase_)));
    new(head_) NodeBase_();
  }

  NodeBase_* head_;
public:
  typedef T value_type;
  typedef T& reference;
  typedef const T& const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef RBTreeBase_::Iterator_<T> iterator;
  typedef RBTreeBase_::ConstIterator_<T> const_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  RBTree() { Init_(); }
  RBTree(const RBTree& tree) {
    Init_();
    CopyTree_(head_, tree.head_);
  }
  RBTree(RBTree&& tree) {
    Init_();
    std::swap(head_, tree.head_);
  }
  ~RBTree() {
    ClearTree_();
    free(head_);
  }

  RBTree& operator=(const RBTree& tree) {
    ClearTree_();
    CopyTree_(head_, tree.head_);
    return *this;
  }
  RBTree& operator=(RBTree&& tree) {
    std::swap(head_, tree.head_);
    return *this;
  }

  iterator begin() { return First_(head_); }
  const_iterator begin() const { return First_(head_); }
  const_iterator cbegin() const { return begin(); }
  iterator end() { return head_; }
  const_iterator end() const { return head_; }
  const_iterator cend() const { return end(); }

  reverse_iterator rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator crbegin() const { return rbegin(); }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }
  const_reverse_iterator crend() const { return rend(); }

#ifdef DEBUG
  void print_() const { Print_(Sup_(head_->left)); puts(""); }
  bool check_() const { return CheckRB_(); }
#endif

  bool empty() const { return !head_->left; }
  size_type size() const { return Size_(head_->left); }

  reference operator[](size_type x) {
    return Sup_(Select_(head_->left, x))->value;
  }
  const_reference operator[](size_type x) const {
    return Sup_(Select_(head_->left, x))->value;
  }
  reference front() { return Sup_(First_(head_->left))->value; }
  const_reference front() const { return Sup_(First_(head_->left))->value; }
  reference back() { return Sup_(Last_(head_->left))->value; }
  const_reference back() const { return Sup_(Last_(head_->left))->value; }
  template <class Pred> iterator partition_bound(Pred func) {
    return PartitionBound_(func);
  }
  template <class Pred> const_iterator partition_bound(Pred func) const {
    return PartitionBound_(func);
  }
  template <class Pred> iterator iter_partition_bound(Pred func) {
    return PartitionBoundIter_(func);
  }
  template <class Pred> const_iterator iter_partition_bound(Pred func) const {
    return PartitionBoundIter_(func);
  }

  void push_back(const T& val) {
    InsertBefore_(head_, GenNode_(val));
  }
  void push_back(T&& val) {
    InsertBefore_(head_, GenNode_(std::move(val)));
  }
  void push_front(const T& val) {
    InsertBefore_(First_(head_), GenNode_(val));
  }
  void push_front(T&& val) {
    InsertBefore_(First_(head_), GenNode_(std::move(val)));
  }
  void pop_back() { FreeNode_(Remove_(Last_(head_->left))); }
  void pop_front() { FreeNode_(Remove_(First_(head_->left))); }
  template <class... Args> void emplace_back(Args&&... args) {
    InsertBefore_(head_, GenNodeArgs_(args...));
  }
  template <class... Args> void emplace_front(Args&&... args) {
    InsertBefore_(First_(head_), GenNodeArgs_(args...));
  }
  iterator insert(iterator it, const T& val) {
    NodeType_* ptr = GenNode_(val);
    InsertBefore_(it.ptr_, ptr);
    return ptr;
  }
  iterator insert(iterator it, T&& val) {
    NodeType_* ptr = GenNode_(std::move(val));
    InsertBefore_(it.ptr_, ptr);
    return ptr;
  }
  template <class... Args> iterator emplace(iterator it, Args&&... args) {
    NodeType_* ptr = GenNodeArgs_(args...);
    InsertBefore_(it.ptr_, ptr);
    return ptr;
  }
  void erase(iterator it) { FreeNode_(Remove_(it.ptr_)); }
  void clear() { ClearTree_(); }

  void swap(RBTree& x) { std::swap(head_, x.head_); }
  void insert_merge(RBTree& tree, const T& val) {
    ConnectLeft_(head_, Merge_(head_->left, GenNode_(val), tree.head_->left));
    tree.head_->left = nullptr;
  }
  void insert_merge(RBTree& tree, T&& val) {
    ConnectLeft_(head_, Merge_(head_->left,
                               GenNode_(std::move(val)), tree.head_->left));
    tree.head_->left = nullptr;
  }
  template <class... Args> void emplace_merge(RBTree& tree, Args&&... args) {
    ConnectLeft_(head_, Merge_(head_->left,
                               GenNodeArgs_(args...), tree.head_->left));
    tree.head_->left = nullptr;
  }
  void merge(RBTree& tree) {
    if (tree.empty()) return;
    if (empty()) { std::swap(head_, tree.head_); return; }
    NodeBase_* pivot = (tree.head_->left->size < head_->left->size) ?
        tree.Remove_(First_(tree.head_->left)) : Remove_(Last_(head_->left));
    ConnectLeft_(head_, Merge_(head_->left, pivot, tree.head_->left));
    tree.head_->left = nullptr;
  }
  RBTree erase_and_split(iterator it) {
    NodeBase_ *l, *r;
    RBTree ret;
    Split_(it.ptr_, l, r, false);
    FreeNode_(it.ptr_);
    ConnectLeft_(head_, l);
    ConnectLeft_(ret.head_, r);
    return ret;
  }
  RBTree split(iterator it) {
    if (it == end()) return RBTree();
    NodeBase_ *l, *r;
    RBTree ret;
    Split_(it.ptr_, l, r, true);
    ConnectLeft_(head_, l);
    ConnectLeft_(ret.head_, r);
    return ret;
  }
};

#endif