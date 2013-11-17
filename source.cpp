#define _USE_MATH_DEFINES
#include <iostream>
#include <string>
#include <map>
#include <ctime>
#include <functional>
#include <vector>
#include <limits>
#include <cassert>
#include <memory>
#include <cmath>
#include <omp.h>
#include <random>
using namespace std;

map<int, int> knownCosts = {
  { 0, 0 },
  { 1, 1 },
  { 2, 2 },
  { 3, 3 },
  { 4, 4 },
  { 5, 5 },
  { 6, 3 },
  { 8, 4 },
  { 15, 5 },
  { 48, 3 },
  { 720, 3 },
  { 8, 4 },
  { 71, 4 },
  { 945, 4 },
  { 15, 5 },
  { 44, 5 },
  { 42, 6 },
  { 729, 6 },
  { 27, 6 }, // sqrt(729)
  { 105, 4 },
  { 216, 6 }, //3!^3!!
};

map<int, int> factorialCache;


auto random = bind(uniform_int_distribution<>(), mt19937());

int factorial(int n)
{
  if (n < 0 || n > 11 || _isnan(n))
    return numeric_limits<int>::quiet_NaN();
  else if (n < 2)
    return 1.0f;
  else if (factorialCache.find(n) != std::end(factorialCache))
    return factorialCache[n];
  else {
    auto result = n * factorial(n - 1);
    factorialCache[n] = result;
    return result;
  }
}

int subFactorial(int n)
{
  if (n < 0 || n > 11 || _isnan(n))
    return numeric_limits<int>::quiet_NaN();
  return floor((factorial(n) / M_E) + 0.5);
}

map<int, int> doubleFactorialCache;
int doubleFactorial(int n)
{
  if (n < 0 || n > 11 || _isnan(n))
    return numeric_limits<int>::quiet_NaN();
  else if (n < 2)
    return 1;
  else if (doubleFactorialCache.find(n) != std::end(doubleFactorialCache))
    return doubleFactorialCache[n];
  else {
    auto result = n * doubleFactorial(n - 2);
    doubleFactorialCache[n] = result;
    return result;
  }
}

typedef function<int(int, int)> binaryFunction;

vector<binaryFunction> binaryFunctions =
{
  [](int a, int b) { return a + b; },
  [](int a, int b) { return a - b; },
  [](int a, int b) { return a*b; },
  [](int a, int b) { return pow(a, b); },
};

string binaryOps{ "+-*^" };

vector<int> terminals = { 1, 2, 3, 4, 5, 6, 8, 15, 42, 44, 48, 71, 216, 105, 720, 729, 945 };

class Node
{
public:
  enum
  {
    Terminal,
    Unary,
    Binary
  } type;

  Node *lhs, *rhs;
  int value;
  int index;

  void Simplify()
  {
    if (type == Unary)
    {
      lhs->Simplify();
      auto z = lhs->Eval();
      if ((z >= 0 && z <= 5) || z == 48 || z == 720)
      {
        type = Terminal;
        value = z;
        delete lhs;
        lhs = nullptr;
      }
    }
    else if (type == Binary)
    {
      lhs->Simplify();
      rhs->Simplify();

      auto u = lhs->Eval();
      if (u >= 0 && u <= 5)
      {
        type = Terminal;
        value = u;
        delete lhs;
        lhs = nullptr;
      }
      auto v = rhs->Eval();
      if (v >= 0 && v <= 5)
      {
        type = Terminal;
        value = v;
        delete rhs;
        rhs = nullptr;
      }
    }
  }

  Node(int* price, int maxDepth, int depth = 0)
  {

    lhs = rhs = nullptr;
    int r = random() % 3;

    // prevent root literals
    if (depth == 0 && r == 0)
      r = 1 + random() % 2;

    assert(depth <= maxDepth);

    if (/*(*price < 4) ||*/ (depth == maxDepth) || (r == 0))
    {
      // terminal
      type = Terminal;
      value = terminals[random() % terminals.size()];
      *price -= value;
      assert(value != 0 && "no way we can have a zero here");
    }
    else if (r == 1)
    {
      // unary
      type = Unary;
      index = random() % 4; // ! or !!

      do {
        if (lhs != nullptr) delete lhs;
        lhs = new Node(price, maxDepth, depth + 1);
      } while (
        (lhs->type == Terminal && lhs->value > 9)
        ||
        (lhs->type == Terminal && lhs->value < 3) // 1-2!/!!
        ||
        (lhs->type == Terminal && lhs->value == 3 && index == 1) // 3!!
        );
    }
    else if (r == 2)
    {
      // binary
      type = Binary;
      index = random() % binaryFunctions.size();
      do
      {
        if (lhs != nullptr) delete lhs;
        if (rhs != nullptr) delete rhs;
        lhs = new Node(price, maxDepth, depth + 1);
        rhs = new Node(price, maxDepth, depth + 1);
      } while (
        ((rhs->type == Terminal) && (binaryOps[index] == '^') && ((lhs->value == 1) || (rhs->value == 1)))
        ||
        ((rhs->type == Terminal) && (rhs->value == 0))
        );
    }

    //Simplify();
  }

  ~Node()
  {
    if (lhs != nullptr) delete lhs;
    if (rhs != nullptr) delete rhs;
  }

  int Eval()
  {
    switch (type)
    {
    case Terminal:
      return value;
    case Unary:
    {
                int z = lhs->Eval();
                switch (index)
                {
                case 0: return factorial(z); break;
                case 1: return doubleFactorial(z); break;
                case 2: return subFactorial(z); break;
                case 3:
                  // square root. really nasty
                  if (z < 0) return numeric_limits<int>::quiet_NaN(); // fail
                  else {
                    float r = sqrtf((float)z);
                    if (r == floor(r))
                      return (int)r;
                    else return numeric_limits<int>::quiet_NaN();
                  }
                }
    }
    case Binary:
      assert(lhs);
      assert(rhs);
      return binaryFunctions[index](lhs->Eval(), rhs->Eval());
    default:
      assert(!"Should not be here!");
    }
  }

  int Cost()
  {
    auto value = Eval();
    if (knownCosts.find(value) != std::end(knownCosts))
      return knownCosts[value];
    else
      switch (type)
    {
      case Terminal:
        return value;
      case Unary:
        return lhs->Cost();
      case Binary:
        return lhs->Cost() + rhs->Cost();
    }
  }

  void Print(bool diagnostic = false)
  {
    int value = Eval();
    if ((value >= 0 && value <= 5) || value == 64 || value == 720)
    {
      cout << value;
      return;
    }

    switch (type)
    {
    case Terminal:
      cout << value;
      break;
    case Unary:
      cout << "(";

      switch (index)
      {
      case 0:
        lhs->Print(diagnostic);
        cout << "!";
        break;
      case 1:
        lhs->Print(diagnostic);
        cout << "!!";
        break;
      case 2:
        cout << "!";
        lhs->Print(diagnostic);
        break;
      }
      cout << ")";
      if (diagnostic)
        cout << "{=" << Eval() << "}";
      break;
    case Binary:
    {
                 cout << "(";
                 lhs->Print(diagnostic);
                 char op = binaryOps[index];
                 assert(op != 0);
                 cout << op;
                 rhs->Print(diagnostic);
                 cout << ")";
                 if (diagnostic)
                   cout << "{=" << Eval() << "}";
                 break;
    }
    default:
      assert(!"Should not be here");
    }
  }
};

void main()
{
  int lastCost = numeric_limits<int>::max();

  for (int i = 1; i <= 11; ++i){
    cout << subFactorial(i) << endl;
  }

  omp_set_num_threads(4);
  omp_lock_t lock;
  omp_init_lock(&lock);

  for (int i = 1;; ++i)
  {
#pragma omp parallel for
    for (int j = 0; j < 1024; ++j)
    {
      int zeros = 13;
      Node node(&zeros, 10);

      auto result = node.Eval();
      int cost;

      if ((result == 105) && ((cost = node.Cost()) <= 20) && (cost <= lastCost))
      {
        omp_set_lock(&lock);
        lastCost = cost;
        //node->Simplify();
        node.Print();
        cout << "=" << result << " cost = " << cost << endl;
        omp_unset_lock(&lock);
      }
    }
  }

  cout << "did I get it?" << endl;
  getchar();
}