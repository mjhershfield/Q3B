#ifndef EXPRSIMPLIFIER_H
#define EXPRSIMPLIFIER_H
#include "z3++.h"
#include <map>
#include <vector>
#include <set>

#include "SimplificationPass.h"
#include "Model.h"

template <>
struct std::hash<z3::expr>
{
    std::size_t operator()(const z3::expr& e) const
    {
	return e.hash();
    }
};

namespace
{
    // Code from boost
    // Reciprocal of the golden ratio helps spread entropy
    //     and handles duplicates.
    // See Mike Seymour in magic-numbers-in-boosthash-combine:
    //     http://stackoverflow.com/questions/4948780

    template <class T>
    inline void hash_combine(std::size_t& seed, T const& v)
    {
	seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }

    // Recursive template code derived from Matthieu M.
    template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
    struct HashValueImpl
    {
	static void apply(size_t& seed, Tuple const& tuple)
	    {
		HashValueImpl<Tuple, Index-1>::apply(seed, tuple);
		hash_combine(seed, std::get<Index>(tuple));
	    }
    };

    template <class Tuple>
    struct HashValueImpl<Tuple,0>
    {
	static void apply(size_t& seed, Tuple const& tuple)
	    {
		hash_combine(seed, std::get<0>(tuple));
	    }
    };
}

template <typename ... TT>
struct std::hash<std::tuple<TT...>>
{
    size_t
    operator()(std::tuple<TT...> const& tt) const
        {
            size_t seed = 0;
            HashValueImpl<std::tuple<TT...> >::apply(seed, tt);
            return seed;
        }

};


class ExprSimplifier
{
public:
    ExprSimplifier(z3::context &ctx) :
	propagateUnconstrained(false),
	goalUnconstrained(false)
    {
      this->context = &ctx;
    }

    ExprSimplifier(z3::context &ctx, bool propagateUnconstrained) :
	propagateUnconstrained(propagateUnconstrained),
	goalUnconstrained(false)
    {
      this->context = &ctx;
    }

    ExprSimplifier(z3::context &ctx, bool propagateUnconstrained, bool goalUnconstrained) :
	propagateUnconstrained(propagateUnconstrained),
	goalUnconstrained(goalUnconstrained)
    {
      this->context = &ctx;
    }

    z3::expr Simplify (z3::expr);
    z3::expr PushQuantifierIrrelevantSubformulas(const z3::expr&);
    z3::expr RefinedPushQuantifierIrrelevantSubformulas(const z3::expr&);
    z3::expr negate(const z3::expr&);
    bool isSentence(const z3::expr&);
    z3::expr PushNegations(const z3::expr&);
    z3::expr CanonizeBoundVariables(const z3::expr&);
    z3::expr DeCanonizeBoundVariables(const z3::expr&);
    z3::expr StripToplevelExistentials(const z3::expr&);
    z3::expr ReduceDivRem(const z3::expr&);

    void SetProduceModels(const bool value)
    {
        produceModels = value;
    }

    void ReconstructModel(Model &model);

private:
    enum BoundType { EXISTENTIAL, UNIVERSAL };

    struct Var
    {
	std::string name;
	BoundType boundType;
	z3::expr expr;

    Var(std::string name, BoundType boundType, z3::expr e) :
	name(name), boundType(boundType), expr(e)
	    {  }
    };

    std::map<const Z3_ast, z3::expr> refinedPushIrrelevantCache;
    std::unordered_map<z3::expr, z3::expr> pushIrrelevantCache;
    std::unordered_map<std::tuple<z3::expr, int, int>, z3::expr> decreaseDeBruijnCache;
    std::unordered_map<std::tuple<z3::expr, int, int>, bool> isRelevantCache;
    std::unordered_map<z3::expr, z3::expr> pushNegationsCache;
    std::unordered_map<std::string, std::string> canonizeVariableRenaming;
    std::unordered_map<z3::expr, bool> isSentenceCache;
    std::unordered_map<z3::expr, z3::expr> reduceDivRemCache;
    void clearCaches();

    z3::context* context;
    z3::expr decreaseDeBruijnIndices(const z3::expr&, int, int);
    bool isRelevant(const z3::expr&, int, int);
    z3::expr mk_or(const z3::expr_vector&) const;
    z3::expr mk_and(const z3::expr_vector&) const ;
    z3::expr modifyQuantifierBody(const z3::expr& quantifierExpr, const z3::expr& newBody) const;
    z3::expr flipQuantifierAndModifyBody(const z3::expr& quantifierExpr, const z3::expr& newBody) const;
    z3::expr applyDer(const z3::expr&) const;

    bool propagateUnconstrained;
    bool goalUnconstrained;
    bool produceModels = false;

    int lastBound = 0;

    std::vector<std::unique_ptr<SimplificationPass>> usedPasses;
};


#endif // EXPRSIMPLIFIER_H
