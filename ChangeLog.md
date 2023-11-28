Tip release (branch [LinkMerge](https://github.com/realthunder/FreeCAD/tree/LinkMerge)) is merged with upstream aa3b2f39 2023.05.22

# Tip 2023.11.02

Introducing new commands `View front face (V, N)` and `View back face (V, B)` (See b6bd57a2 for more details)
TechDraw: fix projection group recompute problem (#901)

# Tip 2023.10.28

Fix view property editing problem (#896)

-- 20231102tip --

# Tip 2023.10.24

Fix git installation in AppImage (affects AddonManager) (#895)
Fix mouse wheel pass through on Windows
Part: fix handling of compound shape in loft (#478)

# Tip 2023.10.18

Add 3D view property 'ThumbnailView' to mark which view is preferred for capturing document thumbnail
Fix undesired behavior when deleting some child object (#877)
Fix crash on right click of view property editor (#829)
Fix pre-selection handling in `Pick geometry` command
Fix thumbnail restore and saving with logo
Support various linked file path resolve mode in App::Link
Part|PartDesign: disable `ValidateShape` by default to avoid slow down on complex shape
PartDesign: fix sketch based profile manual edit
PartDesign: do not auto set UseAllEdges when creating fillet/chamfer
PartDesign: fix UseAllEdges handling in fillet editing task (#883)
Renderer: fix handling of shadow style
Renderer: fix multiple sub-element preselection highlight
Renderer: fix back face picking for objects shown on top
TechDraw: fix restore of dimension when opening document (#812, #845, #868)
TechDraw: fix cosmetic edge deletion (#885)
TechDraw: fix lost of view section when moved outside of the shape bound
TechDraw: refactor background computation
TechDraw: improve selection highlight of various objects
TechDraw: add move by mouse for simple section, detail view, and weld symbol

Second try to fix Python 3.11 not using user site package issue (#864)
Path: fix legacy Controller object backward compatibility issue

# Tip 2023.08.11

Second try to fix Python 3.11 not using user site package issue (#864)
Path: fix legacy Controller object backward compatibility issue

# Tip 2023.08.10

Support user defined document thumbnail through document property ThumbnailFile (cac98c12)
Fix navigation cube corner setting (#855)
Enable user site package for Python 3.11 (#864)
Sketcher: fix editing through App::Link

# Tip 2023.08.06

Fix unit system ordering problem (#859)
Fix workbench enable/disable settings (#852)
Fix navigation cube corner setting (#855)
Fix stylesheet for checkbox with icon in menu
Fix object bounding box restore
Sketcher: support undo constraint dragging
Sketcher: fix missing settings init (#854)
Sketcher: fix external circular edge precision problem (#836, #827)
Renderer: add support for some legacy Coin3D shape nodes (#856)

# Tip 2023.07.30

Fix libfmt packaging problem
PartDesign: fix auto recompute when editing pattern features (#839)

# Tip 2023.07.29

Merged with upstream aa3b2f39 2023.05.22
Fix handling of vendor prefixed (Link branch) user configuration (#824, #833)
Export NaviCube corner size setting through context menu
Allow branding `IssuePage` (#837)
Add new `SplitDark` overlay stylesheet
Improve scene inspector (9d58c46b5)
Implement late 3D picking, used by origin plane
Improve handling of corrupted/incompatible string table in saved file
Use checkbox for checkable menu item
Expose preference package settings in Preference -> General -> Preference pack
Sketcher: fix polyline command toggle (#349)
Sketcher: fix deletion of multiple external edges

# Tip 2023.05.21

Gui: allow child object export override in `Part` container
Gui: allow cancel importer selection
AddonManager: fix macro installer (#822)
Measurement|Part: check for infinite shape (#817)
PartDesign: fix pocket profile normal direction (#816)

# Tip 2023.05.16

Gui: fix API compatibility (#815)
Sketcher: add missing view sketch toolbar button
Sketcher: fix clip plane setup (#810)
Sketcher: fix delete of external element (#814)

# Tip 2023.05.10

Sketcher: fix element selection through task panel (#809)
PartDesign: fix Pad/Pocket creation (#808)
PartDesign: fix Pad/Pocket auto inner taper editing


