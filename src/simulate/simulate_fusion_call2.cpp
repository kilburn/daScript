#include "daScript/misc/platform.h"

#ifdef _MSC_VER
#pragma warning(disable:4505)
#endif

#include "daScript/simulate/simulate_fusion.h"

#if DAS_FUSION

#include "daScript/simulate/sim_policy.h"
#include "daScript/ast/ast_typedecl.h"
#include "daScript/simulate/simulate_visit_op.h"

namespace das {

    struct SimNode_Op2Call2 : SimNode_Op2Fusion {
        virtual SimNode * visit(SimVisitor & vis) override {
            V_BEGIN();
            string name = "Call2";
            name += getSimSourceName(l.type);
            name += getSimSourceName(r.type);
            vis.op(name.c_str());
            if ( fnPtr ) {
                vis.arg(fnPtr->name,"fnPtr");
                vis.arg(fnIndex, "fnIndex");
            }
            if ( cmresEval ) {
                V_SUB(cmresEval);
            }
            l.visit(vis);
            r.visit(vis);
            V_END();
        }
        virtual SimNode * copyNode ( Context & context, NodeAllocator * code ) override {
            SimNode_Op2Call2 * that = (SimNode_Op2Call2 *) SimNode_Op2Fusion::copyNode(context, code);
            if ( fnPtr ) {
                that->fnPtr = context.getFunction(fnIndex);
            }
            return that;
        }
        SimFunction * fnPtr = nullptr;
        int32_t  fnIndex = -1;
        SimNode * cmresEval = nullptr;
    };


/* Call */

#define EVAL_NODE(TYPE,CTYPE)\
        virtual CTYPE eval##TYPE ( Context & context ) override { \
            return cast<CTYPE>::to(compute(context)); \
        }

#define DAS_EVAL_NODE               \
    EVAL_NODE(Ptr,char *);          \
    EVAL_NODE(Int,int32_t);         \
    EVAL_NODE(UInt,uint32_t);       \
    EVAL_NODE(Int64,int64_t);       \
    EVAL_NODE(UInt64,uint64_t);     \
    EVAL_NODE(Float,float);         \
    EVAL_NODE(Double,double);       \
    EVAL_NODE(Bool,bool);

// OP(COMPUTEL,*)
#undef IMPLEMENT_OP2_NODE_ANYR
#define IMPLEMENT_OP2_NODE_ANYR(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_Any_##COMPUTEL : SimNode_Op2Call2 { \
        INLINE vec4f compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            vec4f argValues[2]; \
            argValues[0] = l.subexpr->eval(context); \
            argValues[1] = v_ldu((const float *)r.compute##COMPUTEL(context)); \
            return context.call(fnPtr, argValues, debugInfo.line); \
        } \
        virtual vec4f eval ( Context & context ) override { \
            return compute(context); \
        } \
        DAS_EVAL_NODE \
    };

// OP(*,COMPUTER)
#undef IMPLEMENT_OP2_NODE_ANYL
#define IMPLEMENT_OP2_NODE_ANYL(INLINE,OPNAME,TYPE,CTYPE,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTER##_Any : SimNode_Op2Call2 { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            vec4f argValues[2]; \
            argValues[0] = v_ldu((const float *)l.compute##COMPUTER(context)); \
            argValues[1] = r.subexpr->eval(context); \
            return context.call(fnPtr, argValues, debugInfo.line); \
        } \
        virtual vec4f eval ( Context & context ) override { \
            return compute(context); \
        } \
        DAS_EVAL_NODE \
    };

// OP(COMPUTEL,COMPUTER)
#undef IMPLEMENT_OP2_NODE
#define IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2Call2 { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            vec4f argValues[2]; \
            argValues[0] = v_ldu((const float *)l.compute##COMPUTEL(context)); \
            argValues[1] = v_ldu((const float *)r.compute##COMPUTER(context)); \
            return context.call(fnPtr, argValues, debugInfo.line); \
        } \
        virtual vec4f eval ( Context & context ) override { \
            return compute(context); \
        } \
        DAS_EVAL_NODE \
    };

#undef IMPLEMENT_OP2_SETUP_NODE
#define IMPLEMENT_OP2_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op2Call2 *)result; \
    auto sn = (SimNode_CallBase *)node; \
    rn->fnPtr = sn->fnPtr; \
    rn->fnIndex = sn->fnIndex; \
    rn->cmresEval = sn->cmresEval; \
    rn->baseType = Type::none;

__forceinline SimNode * safeArg2 ( SimNode * node, int index ) {
    auto cb = static_cast<SimNode_CallBase *>(node);
    return (cb->nArguments==2) ? cb->arguments[index] : nullptr;
}

#define FUSION_OP2_SUBEXPR_LEFT(CTYPE,node)     (safeArg2(node,0))
#define FUSION_OP2_SUBEXPR_RIGHT(CTYPE,node)    (safeArg2(node,1))

#include "daScript/simulate/simulate_fusion_op2_impl.h"

IMPLEMENT_ANY_OP2(__forceinline, Call, Ptr, StringPtr)

// OP(COMPUTEL,*)
#undef IMPLEMENT_OP2_NODE_ANYR
#define IMPLEMENT_OP2_NODE_ANYR(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_Any_##COMPUTEL : SimNode_Op2Call2 { \
        __forceinline char * compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto cmres = cmresEval->evalPtr(context); \
            vec4f argValues[2]; \
            argValues[0] = l.subexpr->eval(context); \
            argValues[1] = v_ldu((const float *)r.compute##COMPUTEL(context)); \
            return cast<char *>::to(context.callWithCopyOnReturn(fnPtr, argValues, cmres, debugInfo.line)); \
        } \
        DAS_PTR_NODE; \
    };

// OP(*,COMPUTER)
#undef IMPLEMENT_OP2_NODE_ANYL
#define IMPLEMENT_OP2_NODE_ANYL(INLINE,OPNAME,TYPE,CTYPE,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTER##_Any : SimNode_Op2Call2 { \
        __forceinline char * compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto cmres = cmresEval->evalPtr(context); \
            vec4f argValues[2]; \
            argValues[0] = v_ldu((const float *)l.compute##COMPUTER(context)); \
            argValues[1] = r.subexpr->eval(context); \
            return cast<char *>::to(context.callWithCopyOnReturn(fnPtr, argValues, cmres, debugInfo.line)); \
        } \
        DAS_PTR_NODE; \
    };

// OP(COMPUTEL,COMPUTER)
#undef IMPLEMENT_OP2_NODE
#define IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2Call2 { \
        __forceinline char * compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto cmres = cmresEval->evalPtr(context); \
            vec4f argValues[2]; \
            argValues[0] = v_ldu((const float *)l.compute##COMPUTEL(context)); \
            argValues[1] = v_ldu((const float *)r.compute##COMPUTER(context)); \
            return cast<char *>::to(context.callWithCopyOnReturn(fnPtr, argValues, cmres, debugInfo.line)); \
        } \
        DAS_PTR_NODE; \
    };

#include "daScript/simulate/simulate_fusion_op2_impl.h"

IMPLEMENT_ANY_OP2(__forceinline, CallAndCopyOrMove, Ptr, StringPtr)

    void createFusionEngine_call2() {
        (*g_fusionEngine)["Call"].push_back(make_shared<FusionPoint_Call_StringPtr>());
        (*g_fusionEngine)["CallAndCopyOrMove"].push_back(make_shared<FusionPoint_CallAndCopyOrMove_StringPtr>());
    }
}

#endif



