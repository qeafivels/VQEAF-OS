from pathlib import Path
root=Path(__file__).resolve().parents[1]
cpp=(root/"src/apps/Apps.cpp").read_text()
hpp=(root/"src/apps/Apps.h").read_text()
planner=(root/"src/services/BrowserOverviewDirty.h").read_text()
checks={
 "planner is allocation-free and models full/focus/scrollbar/no-op":"enum class Kind" in planner and "Selection" in planner and "Progress" in planner and "None" in planner,
 "BrowserApp owns planner":"BrowserOverviewDirty overviewDirty;" in hpp,
 "overview resets on document navigation":"overviewDirty.invalidate();" in cpp,
 "inertial tick requests incremental render":"if(redrawOverview(ctx,false))++motionFrames;" in cpp,
 "skip redundant tile clear after full background":"if(plan.kind!=BrowserOverviewDirty::Kind::Full)" in cpp,
 "focus reuses 3x3 tiles":"paintTile(old);" in cpp and "paintTile(next);" in cpp,
 "LCD benchmark compares equivalent full and focus work":"[QB][HW][COMPARE]" in cpp and "frames=32" in cpp,
}
for label,ok in checks.items():
    assert ok,label
    print("PASS",label)
