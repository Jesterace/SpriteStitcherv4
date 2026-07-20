#include "ss/core/pattern/PatternModel.h"
#include "ss/core/undo/PatternCommands.h"

#include <QUndoStack>

#include <cstdio>

using namespace ss::core;

namespace {

int failures = 0;

void expect(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

} // namespace

int main() {
    // RecolorCellCommand: basic redo/undo round trip.
    {
        pattern::PatternModel model;
        model.reset(3, 3, "test");
        model.setCellColor(1, 1, "310");

        QUndoStack stack;
        stack.push(new undo::RecolorCellCommand(&model, 1, 1, "700"));
        expect(model.cellAt(1, 1).dmcCode == "700", "recolor: redo did not apply new color");

        stack.undo();
        expect(model.cellAt(1, 1).dmcCode == "310", "recolor: undo did not restore old color");

        stack.redo();
        expect(model.cellAt(1, 1).dmcCode == "700", "recolor: redo (again) did not reapply new color");
    }

    // SwapColorCommand: straightforward case.
    {
        pattern::PatternModel model;
        model.reset(2, 2, "test");
        model.setCellColor(0, 0, "310");
        model.setCellColor(1, 0, "310");
        model.setCellColor(0, 1, "700");
        model.setCellColor(1, 1, "700");

        QUndoStack stack;
        stack.push(new undo::SwapColorCommand(&model, "310", "801"));
        expect(model.cellAt(0, 0).dmcCode == "801", "swap: redo did not apply to (0,0)");
        expect(model.cellAt(1, 0).dmcCode == "801", "swap: redo did not apply to (1,0)");
        expect(model.cellAt(0, 1).dmcCode == "700", "swap: redo touched an unrelated cell");

        stack.undo();
        expect(model.cellAt(0, 0).dmcCode == "310", "swap: undo did not restore (0,0)");
        expect(model.cellAt(1, 0).dmcCode == "310", "swap: undo did not restore (1,0)");
        expect(model.cellAt(0, 1).dmcCode == "700", "swap: undo touched an unrelated cell");
    }

    // SwapColorCommand: the tricky case — target color already exists
    // elsewhere in the pattern before the swap. A naive "reverse swap"
    // undo would incorrectly touch those pre-existing cells too.
    {
        pattern::PatternModel model;
        model.reset(3, 1, "test");
        model.setCellColor(0, 0, "310"); // will be swapped to 801
        model.setCellColor(1, 0, "801"); // already 801 before the swap — must stay untouched
        model.setCellColor(2, 0, "700"); // unrelated

        QUndoStack stack;
        stack.push(new undo::SwapColorCommand(&model, "310", "801"));
        expect(model.cellAt(0, 0).dmcCode == "801", "swap-preexisting: redo did not apply to (0,0)");
        expect(model.cellAt(1, 0).dmcCode == "801", "swap-preexisting: (1,0) should still be 801");

        stack.undo();
        expect(model.cellAt(0, 0).dmcCode == "310", "swap-preexisting: undo did not restore (0,0) to 310");
        expect(model.cellAt(1, 0).dmcCode == "801",
               "swap-preexisting: undo incorrectly reverted the pre-existing 801 cell");
        expect(model.cellAt(2, 0).dmcCode == "700", "swap-preexisting: undo touched an unrelated cell");
    }

    if (failures) {
        std::fprintf(stderr, "%d failures\n", failures);
        return 1;
    }
    std::printf("All undo tests passed.\n");
    return 0;
}
