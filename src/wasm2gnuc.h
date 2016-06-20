using namespace wasm;
using namespace cashew;

std::string toString(int64_t i)
{
  char *buf = (char *)malloc(1024);
  
  sprintf(buf, "%lu", (unsigned long)i);
  
  return buf;
}

std::string toString(double x)
{
  char *buf = (char *)malloc(1024);
  
  sprintf(buf, "%f", x);
  
  return buf;
}

struct CVisitor : public Visitor<CVisitor, std::string> {
  std::string visitBlock(Block *curr)
  {
    std::string ret = "({";
    std::string x = "x";
    if (curr->name.str) {
      x = curr->name.str;
      x += "x";
    }
    size_t i = 0;
    if (curr->type != none && curr->type != unreachable)
      ret += printWasmType(curr->type) + std::string(" ") + x + ";\n";

    for (auto expr : curr->list) {
      if (++i == curr->list.size() && curr->type != none && curr->type != unreachable) {
        ret += x + " = ";
      }
      ret += visit(expr);
      ret += ";\n";
    }
    if (curr->name.str) {
      ret += curr->name.str;
      ret += ":;";
    }
    if (curr->type != none && curr->type != unreachable)
      ret += x + ";\n";
    ret += "})";

    return ret;
  }

  std::string visitIf(If *curr)
  {
    std::string ret = "({";
    if (curr->type != unreachable && curr->type != none) {
      ret += printWasmType(curr->type);
      ret += " x;";
    }
    ret += "if (";
    ret += visit(curr->condition);
    ret += ") ";
    if (curr->type != unreachable && curr->type != none &&
        curr->ifTrue->type != unreachable && curr->ifTrue->type != none)
      ret += "x = ";
    ret += visit(curr->ifTrue);
    ret += ";";
    if (curr->ifFalse) {
      ret += "else ";
      if (curr->type != unreachable && curr->type != none &&
          curr->ifFalse->type != unreachable && curr->ifFalse->type != none)
        ret += "x = ";
      ret += visit(curr->ifFalse);
      ret += ";";
    }
    if (curr->type != unreachable && curr->type != none)
      ret += "x;";
    ret += "})";

    return ret;
  }

  std::string visitLoop(Loop *curr)
  {
    std::string ret = "({";
    if (curr->type != unreachable && curr->type != none) {
      ret += printWasmType(curr->type);
      ret += " x;";
    }
    ret += std::string(curr->in.str) + ":";
    if (curr->type != unreachable && curr->type != none)
      ret += "x = ";
    ret += visit(curr->body);
    ret += ";";
    ret += std::string(curr->out.str) + ":";
    if (curr->type != unreachable && curr->type != none)
      ret += "x = ";
    if (curr->type != unreachable && curr->type != none)
      ret += "x;";
    else
      ret += ";";
    ret += "})";

    return ret;
  }

  std::string visitBreak(Break *curr)
  {
    std::string ret = "({";
    if (curr->condition) {
      ret += "if (";
      ret += visit(curr->condition);
      ret += ")";
    }
    if (curr->value) {
      ret += "x" + std::string(curr->name.str) + " = " + visit(curr->value) + ";";
    }
    ret += "goto ";
    ret += curr->name.str;
    ret += ";";
    ret += "})";

    return ret;
  }

  std::string visitSwitch(Switch *curr)
  {
    std::string ret = "({";
    ret += "switch (";
    ret += visit(curr->condition);
    ret += ") {";
    size_t i = 0;
    for (i = 0; i < curr->targets.size(); i++) {
      ret += "case ";
      ret += toString((int64_t)i);
      ret += ":\n";
      ret += "goto ";
      ret += curr->targets[i].str;
      ret += ";";
    }
    ret += "default:\n";
    ret += "goto ";
    ret += curr->default_.str;
    ret += ";";
    ret += "}"; ret += "})";
    return ret;
  }
  std::string visitCall(Call *curr)
  {
    std::string ret = "({";
    size_t i = 0;

    ret += curr->target.str;
    ret += "(";

    for (auto op : curr->operands) {
      ret += visit(op);
      if (++i < curr->operands.size())
        ret += ", ";
    }

    ret += ")";
    ret += ";";
    ret += "})";

    return ret;
  }

  std::string visitCallImport(CallImport *curr)
  {
    std::string ret = "({";
    size_t i = 0;

    ret += curr->target.str;
    ret += "(";

    for (auto op : curr->operands) {
      ret += visit(op);
      if (++i < curr->operands.size())
        ret += ", ";
    }

    ret += ")";
    ret += ";";
    ret += "})";

    return ret;
  }
  std::string visitCallIndirect(CallIndirect *curr)
  {
    return "";
  }
  std::string visitGetLocal(GetLocal *curr)
  {
    std::string ret = "({";

    ret += "var$";
    ret += toString((int64_t)curr->index);
    ret += ";";
    ret += "})";

    return ret;
  }

  std::string visitSetLocal(SetLocal *curr)
  {
    std::string ret = "({";

    ret += "var$";
    ret += toString((int64_t)curr->index);
    ret += " = ";
    ret += visit(curr->value);
    ret += ";";
    ret += "})";

    return ret;
  }

  std::string visitLoad(Load *curr)
  {
    std::string ret = "({";
    ret += "load_";
    ret += printWasmType(curr->type);
    ret += "_";
    ret += printWasmType(curr->type)[0];
    ret += toString(int64_t(curr->bytes * 8));
    if (printWasmType(curr->type)[0] == 'i') {
      ret += "_";
      ret += (curr->signed_) ? "s" : "u";
    }
    ret += "(";
    ret += visit(curr->ptr);
    ret += ");";
    ret += "})";
    return ret;
  }
  std::string visitStore(Store *curr) {
    std::string ret = "({";
    ret += "store_";
    ret += printWasmType(curr->type);
    ret += "_";
    ret += printWasmType(curr->type)[0];
    ret += toString(int64_t(curr->bytes * 8));
    ret += "(";
    ret += visit(curr->ptr);
    ret += ", ";
    ret += visit(curr->value);
    ret += ");";
    ret += "})";
    return ret;
  }
  std::string visitConst(Const *curr)
  {
    std::string ret = "(";
    ret += "(" + std::string(printWasmType(curr->type)) + ")";
    switch (curr->value.type) {
    case f32: ret += toString((double)curr->value.getf32()); break;
    case f64: ret += toString((double)curr->value.getf64()); break;
    case i32: ret += toString((int64_t)curr->value.geti32()) + "UL"; break;
    case i64: ret += toString((int64_t)curr->value.geti64()) + "UL"; break;
    case unreachable:
    case none: ret += "({ ; })";
    }
    ret += ")";
    return ret;
  }
  std::string visitUnary(Unary *curr)
  {
    std::string ret = "(";
    switch (curr->op) {
      case ClzInt32:               ret += "i32_clz";     break;
      case CtzInt32:               ret += "i32_ctz";     break;
      case PopcntInt32:            ret += "i32_popcnt";  break;
      case EqZInt32:               ret += "i32_eqz";     break;
      case ClzInt64:               ret += "i64_clz";     break;
      case CtzInt64:               ret += "i64_ctz";     break;
      case PopcntInt64:            ret += "i64_popcnt";  break;
      case EqZInt64:               ret += "i64_eqz";     break;
      case NegFloat32:             ret += "f32_neg";     break;
      case AbsFloat32:             ret += "f32_abs";     break;
      case CeilFloat32:            ret += "f32_ceil";    break;
      case FloorFloat32:           ret += "f32_floor";   break;
      case TruncFloat32:           ret += "f32_trunc";   break;
      case NearestFloat32:         ret += "f32_nearest"; break;
      case SqrtFloat32:            ret += "f32_sqrt";    break;
      case NegFloat64:             ret += "f64_neg";     break;
      case AbsFloat64:             ret += "f64_abs";     break;
      case CeilFloat64:            ret += "f64_ceil";    break;
      case FloorFloat64:           ret += "f64_floor";   break;
      case TruncFloat64:           ret += "f64_trunc";   break;
      case NearestFloat64:         ret += "f64_nearest"; break;
      case SqrtFloat64:            ret += "f64_sqrt";    break;
      case ExtendSInt32:           ret += "i64_extend_s_i32"; break;
      case ExtendUInt32:           ret += "i64_extend_u_i32"; break;
      case WrapInt64:              ret += "i32_wrap_i64"; break;
      case TruncSFloat32ToInt32:   ret += "i32_trunc_s_f32"; break;
      case TruncSFloat32ToInt64:   ret += "i64_trunc_s_f32"; break;
      case TruncUFloat32ToInt32:   ret += "i32_trunc_u_f32"; break;
      case TruncUFloat32ToInt64:   ret += "i64_trunc_u_f32"; break;
      case TruncSFloat64ToInt32:   ret += "i32_trunc_s_f64"; break;
      case TruncSFloat64ToInt64:   ret += "i64_trunc_s_f64"; break;
      case TruncUFloat64ToInt32:   ret += "i32_trunc_u_f64"; break;
      case TruncUFloat64ToInt64:   ret += "i64_trunc_u_f64"; break;
      case ReinterpretFloat32:     ret += "i32_reinterpret_f32"; break;
      case ReinterpretFloat64:     ret += "i64_reinterpret_f64"; break;
      case ConvertUInt32ToFloat32: ret += "f32_convert_u_i32"; break;
      case ConvertUInt32ToFloat64: ret += "f64_convert_u_i32"; break;
      case ConvertSInt32ToFloat32: ret += "f32_convert_s_i32"; break;
      case ConvertSInt32ToFloat64: ret += "f64_convert_s_i32"; break;
      case ConvertUInt64ToFloat32: ret += "f32_convert_u_i64"; break;
      case ConvertUInt64ToFloat64: ret += "f64_convert_u_i64"; break;
      case ConvertSInt64ToFloat32: ret += "f32_convert_s_i64"; break;
      case ConvertSInt64ToFloat64: ret += "f64_convert_s_i64"; break;
      case PromoteFloat32:         ret += "f64_promote_f32"; break;
      case DemoteFloat64:          ret += "f32_demote_f64"; break;
      case ReinterpretInt32:       ret += "f32_reinterpret_i32"; break;
      case ReinterpretInt64:       ret += "f64_reinterpret_i64"; break;
    default:
      ret += "BROKEN";
      WASM_UNREACHABLE();
    }
    ret += "(";
    ret += visit(curr->value);
    ret += ")";
    ret += ")";
    return ret;
  }
  std::string visitBinary(Binary *curr)
  {
    std::string ret = "(";
    switch (curr->op) {
      case AddInt32:      ret += "i32_add";      break;
      case SubInt32:      ret += "i32_sub";      break;
      case MulInt32:      ret += "i32_mul";      break;
      case DivSInt32:     ret += "i32_div_s";    break;
      case DivUInt32:     ret += "i32_div_u";    break;
      case RemSInt32:     ret += "i32_rem_s";    break;
      case RemUInt32:     ret += "i32_rem_u";    break;
      case AndInt32:      ret += "i32_and";      break;
      case OrInt32:       ret += "i32_or";       break;
      case XorInt32:      ret += "i32_xor";      break;
      case ShlInt32:      ret += "i32_shl";      break;
      case ShrUInt32:     ret += "i32_shr_u";    break;
      case ShrSInt32:     ret += "i32_shr_s";    break;
      case RotLInt32:     ret += "i32_rotl";     break;
      case RotRInt32:     ret += "i32_rotr";     break;
      case EqInt32:       ret += "i32_eq";       break;
      case NeInt32:       ret += "i32_ne";       break;
      case LtSInt32:      ret += "i32_lt_s";     break;
      case LtUInt32:      ret += "i32_lt_u";     break;
      case LeSInt32:      ret += "i32_le_s";     break;
      case LeUInt32:      ret += "i32_le_u";     break;
      case GtSInt32:      ret += "i32_gt_s";     break;
      case GtUInt32:      ret += "i32_gt_u";     break;
      case GeSInt32:      ret += "i32_ge_s";     break;
      case GeUInt32:      ret += "i32_ge_u";     break;

      case AddInt64:      ret += "i64_add";      break;
      case SubInt64:      ret += "i64_sub";      break;
      case MulInt64:      ret += "i64_mul";      break;
      case DivSInt64:     ret += "i64_div_s";    break;
      case DivUInt64:     ret += "i64_div_u";    break;
      case RemSInt64:     ret += "i64_rem_s";    break;
      case RemUInt64:     ret += "i64_rem_u";    break;
      case AndInt64:      ret += "i64_and";      break;
      case OrInt64:       ret += "i64_or";       break;
      case XorInt64:      ret += "i64_xor";      break;
      case ShlInt64:      ret += "i64_shl";      break;
      case ShrUInt64:     ret += "i64_shr_u";    break;
      case ShrSInt64:     ret += "i64_shr_s";    break;
      case RotLInt64:     ret += "i64_rotl";     break;
      case RotRInt64:     ret += "i64_rotr";     break;
      case EqInt64:       ret += "i64_eq";       break;
      case NeInt64:       ret += "i64_ne";       break;
      case LtSInt64:      ret += "i64_lt_s";     break;
      case LtUInt64:      ret += "i64_lt_u";     break;
      case LeSInt64:      ret += "i64_le_s";     break;
      case LeUInt64:      ret += "i64_le_u";     break;
      case GtSInt64:      ret += "i64_gt_s";     break;
      case GtUInt64:      ret += "i64_gt_u";     break;
      case GeSInt64:      ret += "i64_ge_s";     break;
      case GeUInt64:      ret += "i64_ge_u";     break;

      case AddFloat32:      ret += "f32_add";      break;
      case SubFloat32:      ret += "f32_sub";      break;
      case MulFloat32:      ret += "f32_mul";      break;
      case DivFloat32:      ret += "f32_div";      break;
      case CopySignFloat32: ret += "f32_copysign"; break;
      case MinFloat32:      ret += "f32_min";      break;
      case MaxFloat32:      ret += "f32_max";      break;
      case EqFloat32:       ret += "f32_eq";       break;
      case NeFloat32:       ret += "f32_ne";       break;
      case LtFloat32:       ret += "f32_lt";       break;
      case LeFloat32:       ret += "f32_le";       break;
      case GtFloat32:       ret += "f32_gt";       break;
      case GeFloat32:       ret += "f32_ge";       break;

      case AddFloat64:      ret += "f64_add";      break;
      case SubFloat64:      ret += "f64_sub";      break;
      case MulFloat64:      ret += "f64_mul";      break;
      case DivFloat64:      ret += "f64_div";      break;
      case CopySignFloat64: ret += "f64_copysign"; break;
      case MinFloat64:      ret += "f64_min";      break;
      case MaxFloat64:      ret += "f64_max";      break;
      case EqFloat64:       ret += "f64_eq";       break;
      case NeFloat64:       ret += "f64_ne";       break;
      case LtFloat64:       ret += "f64_lt";       break;
      case LeFloat64:       ret += "f64_le";       break;
      case GtFloat64:       ret += "f64_gt";       break;
      case GeFloat64:       ret += "f64_ge";       break;

      default:       abort();
      ret += "BROKEN";
      WASM_UNREACHABLE();
    }
    ret += "(";
    ret += visit(curr->left);
    ret += ", ";
    ret += visit(curr->right);
    ret += ")";
    ret += ")";
    return ret;
  }
  std::string visitSelect(Select *curr)
  {
    std::string ret = "(";

    ret += "(";
    ret += visit(curr->condition);
    ret += ")";
    ret += "?";
    ret += "(";
    ret += visit(curr->ifTrue);
    ret += ")";
    ret += ":";
    ret += "(";
    ret += visit(curr->ifFalse);
    ret += ")";

    ret += ")";

    return ret;
  }

  std::string visitReturn(Return *curr)
  {
    std::string ret = "";

    ret += "return ";
    ret += visit(curr->value);
    ret += ";";

    return ret;
  }

  std::string visitHost(Host *curr)
  {
    std::string ret = "BROKEN";

    return ret;
  }
  std::string visitNop(Nop *curr)
  {
    std::string ret = "({ ; })";

    return ret;
  }
  std::string visitUnreachable(Unreachable *curr)
  {
    std::string ret = "({ __builtin_unreachable(); })";

    return ret;
  }
  // Module-level visitors
  std::string visitFunctionType(FunctionType *curr) { return ""; }
  std::string visitImport(Import *curr) { return ""; }
  std::string visitExport(Export *curr) { return ""; }
  std::string visitFunctionPrototype(Function *curr)
  {
    std::string ret = "\n\n";

    if (curr->result == none)
      ret += "void";
    else
      ret += printWasmType(curr->result);
    ret += " ";
    ret += std::string(curr->name.str);
    ret += "(";
    Index i = 0;
    for (auto param : curr->params) {
      if (i != 0)
        ret += ", ";
      ret += printWasmType(param);
      ret += " ";
      ret += curr->localNames[i++].str;
    }
    ret += ");";
    ret += "\n";

    return ret;
  }
  
  std::string visitFunctionSwitchStmt(Function *curr)
  {
    if (strncmp(curr->name.str, "f_", 2) ||
        !strcmp(curr->name.str, "f_indcall"))
      return std::string("");

    std::string ret = "";

    ret += "case ";
    ret += curr->name.str + 2;
    ret += "UL:\n";
    ret += "return ";
    ret += curr->name.str;
    ret += "(dpc, sp, r0, r1, rpc, pc0);\n";

    return ret;
  }
  
  std::string visitFunction(Function *curr)
  {
    std::string ret = "\n\n";

    if (curr->result == none)
      ret += "void";
    else
      ret += printWasmType(curr->result);
    ret += " ";
    ret += std::string(curr->name.str);
    ret += "(";
    Index i = 0;
    for (auto param : curr->params) {
      if (i != 0)
        ret += ", ";
      ret += printWasmType(param);
      ret += " ";
      ret += curr->localNames[i++].str;
    }
    ret += ")";
    ret += "\n";

    ret += "{";
    for (auto local : curr->vars) {
      ret += printWasmType(local);
      ret += " ";
      ret += curr->localNames[i++].str;
      ret += ";\n";
    }
    ret += visit(curr->body);
    ret += ";";
    ret += "}";

    return ret;
  }
  std::string visitTable(Table *curr) { return ""; }
  std::string visitMemory(Memory *curr) { return ""; }
  std::string visitModule(Module *curr)
  {
    std::string ret;
    for (auto & function : curr->functions)
      ret += visitFunctionPrototype(&*function);
    for (auto & function : curr->functions)
      ret += visitFunction(&*function);

    ret += "\n\ni64 import$2(i64 dpc, i64 sp, i64 r0, i64 r1, i64 rpc, i64 pc0)\n";
    ret += "{\n";
    ret += "switch(pc0<<4ULL) {\n";
    for (auto & function : curr->functions)
      ret += visitFunctionSwitchStmt(&*function);
    ret += "default:\nfor(;;);\n";
    ret += "}\n";
    ret += "}\n";

    return ret;
  }
};
