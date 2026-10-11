// Ghidra script: give the game logic of mm2run.exe (2002 build) the engine's
// types, then decompile the game region (0x401000-0x476000) to one C file.
// With the types, the decompiler keeps the arguments of virtual calls on
// variables and shows message fields by name.
//
// usage: analyzeHeadless <proj> mm2 -process mm2run.exe -noanalysis \
//          -postScript ApplyEngineTypes.java <output .c>
//@category MM2
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.app.cmd.function.ApplyFunctionSignatureCmd;
import ghidra.program.model.address.*;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.SourceType;
import java.io.*;
import java.util.*;

public class ApplyEngineTypes extends GhidraScript {
	DataTypeManager dtm;
	CategoryPath cat = new CategoryPath("/mm2");

	DataType INT = IntegerDataType.dataType, UINT = UnsignedIntegerDataType.dataType,
		USHORT = UnsignedShortDataType.dataType, SHORT = ShortDataType.dataType, VOID = VoidDataType.dataType;

	FunctionDefinitionDataType fdef(String name, DataType ret, Object... params) {
		FunctionDefinitionDataType f = new FunctionDefinitionDataType(cat, name, dtm);
		f.setReturnType(ret);
		List<ParameterDefinition> ps = new ArrayList<>();
		for (int i = 0; i < params.length; i += 2)
			ps.add(new ParameterDefinitionImpl((String) params[i], (DataType) params[i + 1], null));
		f.setArguments(ps.toArray(new ParameterDefinition[0]));
		try {
			f.setCallingConvention("__thiscall");
		} catch (Exception e) {
			throw new RuntimeException(e);
		}
		return f;
	}

	void sig(long addr, String name, DataType ret, Object... params) throws Exception {
		Address a = toAddr(addr);
		Function fn = getFunctionAt(a);
		if (fn == null)
			fn = createFunction(a, name);
		if (fn == null) {
			println("no function at " + a);
			return;
		}
		fn.setName(name, SourceType.USER_DEFINED);
		FunctionDefinitionDataType f = fdef(name, ret, params);
		new ApplyFunctionSignatureCmd(a, f, SourceType.USER_DEFINED).applyTo(currentProgram, monitor);
	}

	@Override
	public void run() throws Exception {
		String out = getScriptArgs()[0];
		dtm = currentProgram.getDataTypeManager();
		int tx = currentProgram.startTransaction("mm2 types");

		// --- structures ---
		StructureDataType msg = new StructureDataType(cat, "Message", 0x2c, dtm);
		msg.replaceAtOffset(0x00, new PointerDataType(VOID), 4, "vtbl", null);
		msg.replaceAtOffset(0x04, USHORT, 2, "target", null);
		msg.replaceAtOffset(0x08, INT, 4, "mode", null);
		msg.replaceAtOffset(0x0c, INT, 4, "cmd", null);
		msg.replaceAtOffset(0x10, USHORT, 2, "msg", null);
		msg.replaceAtOffset(0x14, INT, 4, "handled", null);
		msg.replaceAtOffset(0x18, INT, 4, "p1", null);
		msg.replaceAtOffset(0x1c, INT, 4, "p2", null);
		msg.replaceAtOffset(0x20, INT, 4, "p3", null);
		msg.replaceAtOffset(0x24, INT, 4, "result", null);
		msg.replaceAtOffset(0x28, INT, 4, "next", null);
		DataType msgT = dtm.addDataType(msg, DataTypeConflictHandler.REPLACE_HANDLER);
		DataType msgP = new PointerDataType(msgT);

		StructureDataType var = new StructureDataType(cat, "Var", 0x1c, dtm);
		DataType varT = dtm.addDataType(var, DataTypeConflictHandler.REPLACE_HANDLER);
		DataType varP = new PointerDataType(varT);
		StructureDataType vvt = new StructureDataType(cat, "VarVtbl", 0x60, dtm);
		String[] cmpNames = { "eq", "gt", "lt", "ge", "le" };
		for (int i = 0; i < 5; i++)
			vvt.replaceAtOffset(0x0c + 4 * i, new PointerDataType(fdef("Var_" + cmpNames[i], BooleanDataType.dataType, "other", varP)), 4, cmpNames[i], null);
		vvt.replaceAtOffset(0x20, new PointerDataType(fdef("Var_setRaw", VOID, "v", INT)), 4, "setRaw", null);
		vvt.replaceAtOffset(0x24, new PointerDataType(fdef("Var_setString", VOID, "s", new PointerDataType(CharDataType.dataType))), 4, "setString", null);
		vvt.replaceAtOffset(0x28, new PointerDataType(fdef("Var_assignString", varP, "s", new PointerDataType(CharDataType.dataType))), 4, "assignString", null);
		vvt.replaceAtOffset(0x2c, new PointerDataType(fdef("Var_set", VOID, "v", INT)), 4, "set", null);
		vvt.replaceAtOffset(0x30, new PointerDataType(fdef("Var_add", varP, "n", INT)), 4, "add", null);
		vvt.replaceAtOffset(0x34, new PointerDataType(fdef("Var_sub", varP, "n", INT)), 4, "sub", null);
		vvt.replaceAtOffset(0x38, new PointerDataType(fdef("Var_get", INT)), 4, "get", null);
		vvt.replaceAtOffset(0x3c, new PointerDataType(fdef("Var_toString", new PointerDataType(VOID), "out", new PointerDataType(VOID))), 4, "toString", null);
		vvt.replaceAtOffset(0x40, new PointerDataType(fdef("Var_at", varP, "key", INT)), 4, "at", null);
		vvt.replaceAtOffset(0x44, new PointerDataType(fdef("Var_remove", VOID, "key", INT)), 4, "remove", null);
		vvt.replaceAtOffset(0x48, new PointerDataType(fdef("Var_getInt", INT)), 4, "getInt", null);
		vvt.replaceAtOffset(0x4c, new PointerDataType(fdef("Var_x4c", INT)), 4, "x4c", null);
		vvt.replaceAtOffset(0x50, new PointerDataType(fdef("Var_x50", INT, "a", INT)), 4, "x50", null);
		vvt.replaceAtOffset(0x54, new PointerDataType(fdef("Var_x54", INT, "a", INT, "b", INT)), 4, "x54", null);
		vvt.replaceAtOffset(0x58, new PointerDataType(fdef("Var_x58", INT)), 4, "x58", null);
		vvt.replaceAtOffset(0x5c, new PointerDataType(fdef("Var_x5c", INT, "a", INT, "b", INT)), 4, "x5c", null);
		DataType vvtT = dtm.addDataType(vvt, DataTypeConflictHandler.REPLACE_HANDLER);
		Structure varS = (Structure) varT;
		varS.replaceAtOffset(0, new PointerDataType(vvtT), 4, "vtbl", null);

		StructureDataType obj = new StructureDataType(cat, "GameObject", 0x1c, dtm);
		obj.replaceAtOffset(0x00, new PointerDataType(VOID), 4, "vtbl", null);
		obj.replaceAtOffset(0x04, USHORT, 2, "id", null);
		obj.replaceAtOffset(0x08, INT, 4, "param", null);
		obj.replaceAtOffset(0x0c, new PointerDataType(VOID), 4, "world", null);
		obj.replaceAtOffset(0x10, new PointerDataType(VOID), 4, "vars", null);
		obj.replaceAtOffset(0x14, new PointerDataType(VOID), 4, "children", null);
		obj.replaceAtOffset(0x18, UINT, 4, "eventMask", null);
		DataType objT = dtm.addDataType(obj, DataTypeConflictHandler.REPLACE_HANDLER);
		DataType objP = new PointerDataType(objT);
		DataType childP = new PointerDataType(VOID);

		// --- engine API (see docs/ENGINE.md) ---
		sig(0x476910, "Object_ctor", objP, "id", USHORT, "world", new PointerDataType(VOID), "param", INT);
		sig(0x476a60, "post", VOID, "target", USHORT, "msg", USHORT, "p1", INT, "p2", INT, "p3", INT);
		sig(0x476ad0, "send", INT, "target", USHORT, "msg", USHORT, "p1", INT, "p2", INT, "p3", INT);
		sig(0x476b40, "cmd", INT, "target", USHORT, "command", INT, "child", USHORT, "p1", INT, "p2", INT, "p3", INT);
		sig(0x476bc0, "newMsg", msgP, "target", SHORT, "msg", USHORT, "p1", INT, "p2", INT, "p3", INT);
		sig(0x476c50, "newCmd", msgP, "target", SHORT, "command", INT, "child", USHORT, "p1", INT, "p2", INT, "p3", INT);
		sig(0x476ce0, "addInt", VOID, "id", USHORT, "value", INT, "save", INT);
		sig(0x476e20, "addRandom", VOID, "id", USHORT, "min", INT, "max", INT, "save", INT);
		sig(0x476eb0, "addCycler", VOID, "id", USHORT, "min", INT, "max", INT, "save", INT);
		sig(0x476f40, "addShuffle", VOID, "id", USHORT, "min", INT, "max", INT, "save", INT);
		sig(0x476fd0, "addMap", VOID, "id", USHORT, "save", INT);
		sig(0x477050, "var", varP, "id", USHORT);
		sig(0x477060, "varOf", varP, "id", USHORT, "object", USHORT);
		sig(0x4770a0, "addChild", childP, "child", childP);
		sig(0x4770c0, "getChild", childP, "id", USHORT);
		sig(0x4770d0, "getChildOf", childP, "child", USHORT, "object", USHORT);
		sig(0x477120, "setCursor", VOID, "cursor", UINT);
		sig(0x477140, "startTimer", VOID, "timer", SHORT, "ms", INT, "msg", msgP);
		sig(0x477170, "stopTimer", VOID, "timer", SHORT);
		sig(0x477240, "setKeyFocus", USHORT, "object", USHORT);
		sig(0x4771c0, "saveGame", INT, "file", new PointerDataType(VOID));
		sig(0x4771e0, "loadGame", INT, "file", new PointerDataType(VOID), "version", INT);
		sig(0x477a90, "Child_setCursor", VOID, "cursor", UINT);
		sig(0x477b20, "Bitmap_ctor", childP, "id", USHORT, "transparent", INT, "flags", UINT, "mode", INT);
		sig(0x478140, "Anim_ctor", childP, "id", USHORT, "transparent", INT, "flags", UINT, "param", USHORT);
		sig(0x4786a0, "Hotspot_ctor", childP, "id", USHORT, "flags", UINT);
		sig(0x478e40, "Sound_ctor", childP, "id", USHORT, "loops", USHORT);
		sig(0x479400, "TextRes_ctor", childP, "id", USHORT);

		// --- room/object handlers: slot 5 accepts(msg), slot 6 handle(msg) ---
		long[] vtables = findObjectVtables();
		for (long vt : vtables) {
			long a5 = getInt(toAddr(vt + 0x14)) & 0xffffffffL, a6 = getInt(toAddr(vt + 0x18)) & 0xffffffffL;
			if (a5 < 0x476000)
				sig(a5, String.format("Object%x_accepts", vt), INT, "msg", msgP);
			if (a6 < 0x476000)
				sig(a6, String.format("Object%x_handle", vt), VOID, "msg", msgP);
		}
		currentProgram.endTransaction(tx, true);

		// --- decompile the game region ---
		DecompInterface ifc = new DecompInterface();
		ifc.openProgram(currentProgram);
		int n = 0;
		try (PrintWriter w = new PrintWriter(new FileWriter(out))) {
			for (Function f : currentProgram.getFunctionManager().getFunctions(toAddr(0x401000), true)) {
				if (f.getEntryPoint().getOffset() >= 0x476000)
					break;
				DecompileResults r = ifc.decompileFunction(f, 120, monitor);
				w.println("// ==== " + f.getName() + " @ " + f.getEntryPoint());
				w.println(r != null && r.decompileCompleted() ? r.getDecompiledFunction().getC() : "// decompilation failed");
				n++;
			}
		}
		println("decompiled " + n + " functions -> " + out);
	}

	/** The vtables stored by the setup routine FUN_0043f760 (mov [reg], imm32 after an object ctor). */
	long[] findObjectVtables() throws Exception {
		List<Long> vts = new ArrayList<>();
		String[] lines = getScriptArgs().length > 1 ? getScriptArgs()[1].split(",") : new String[0];
		for (String l : lines)
			vts.add(Long.parseLong(l, 16));
		long[] r = new long[vts.size()];
		for (int i = 0; i < r.length; i++)
			r[i] = vts.get(i);
		return r;
	}
}
