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
using namespace std;


map<int,int> factorialCache;

int factorial(int n)
{
  if (n < 0 || n > 10 || _isnan(n))
    return numeric_limits<int>::quiet_NaN();
  else if (n < 2)
    return 1.0f;
  else if (factorialCache.find(n) != end(factorialCache))
    return factorialCache[n];
  else {
    auto result = n * factorial(n-1);
    factorialCache[n] = result;
    return result;
  }
}

int subFactorial(int n)
{
  if (n < 0 || n > 10 || _isnan(n))
    return numeric_limits<int>::quiet_NaN();
  return floor((factorial(n) / M_E) + 0.5);
}

map<int,int> doubleFactorialCache;
int doubleFactorial(int n)
{
  if (n < 0 || n > 10 || _isnan(n))
    return numeric_limits<int>::quiet_NaN();
  else if (n < 2)  
    return 1;
  else if (doubleFactorialCache.find(n) != end(doubleFactorialCache))
    return doubleFactorialCache[n];
  else {
    auto result = n * doubleFactorial(n-2);
    doubleFactorialCache[n] = result;
    return result;
  }
}

typedef function<int(int,int)> binaryFunction;

vector<binaryFunction> binaryFunctions = 
{
  [](int a, int b) { return a + b; },
  [](int a, int b) { return a - b; },
  [](int a, int b) { return a*b; },
  pow<int,int>
};

string binaryOps{ "+-*^" };

vector<int> terminals = { 1, 2, 3, 4, 5, 6, 8, 15, 42, 44, 48, 71, 720, 729, 945 };

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
    int r = rand() % 3;

    // prevent root literals
    if (depth == 0 && r == 0)
      r = 1 + rand() % 2;

    assert(depth <= maxDepth);

    if (/*(*price < 4) ||*/ (depth == maxDepth) || (r == 0)) 
    {
      // terminal
      type = Terminal;
      value = terminals[rand() % terminals.size()];
      *price -= value;
    }
    else if (r == 1)
    {
      // unary
      type = Unary;
      index = rand() % 2; // ! or !!

      do {
        if (lhs != nullptr) delete lhs;
        lhs = new Node(price, maxDepth, depth+1);
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
      index = rand() % binaryFunctions.size();
      do
      {
        if (lhs != nullptr) delete lhs;
        if (rhs != nullptr) delete rhs;
        lhs = new Node(price, maxDepth, depth+1);
        rhs = new Node(price, maxDepth, depth+1);
      } while (
        ((rhs->type == Terminal) && (rhs->value == 1) && (binaryOps[index] == '^'))
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
    if (value == 0.0)
      return 0; // having zero is, in most cases, meaningless
    else if (value == 1)
      return 1;
    else if (value == 2)
      return 2;
    else if (value == 3 || value == 6 || value == 48 || value == 720)
      return 3;
    else if (value == 8 || value == 71 || value == 945)
      return 4;
    else if (value == 15 || value == 44)
      return 5;
    else if (value == 42 || value == 729)
      return 6;
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

map<int, int> knownCosts = {};

void main()
{
  srand((unsigned)time(0));

  int lastCost = numeric_limits<int>::max();

  for (int i = 1; i <= 11; ++i){
    cout << subFactorial(i) << endl;
  }

  omp_lock_t lock;
  omp_init_lock(&lock);

  for (int i = 1; ; ++i)
  {
#pragma omp parallel for
    for (int j = 0; j < 4; ++j)
    {
      int zeros = 13;
      Node node(&zeros, 10);

      auto result = node.Eval();
      int cost;

      if ((result == 2008) && ((cost = node.Cost()) <= 13) && (cost < lastCost))
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