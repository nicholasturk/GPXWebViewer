const ffi = require("ffi-napi");
const path = require("path");
const ref = require("ref-napi");
const voidPtr = ref.refType(ref.types.void);
var ArrayType = require("ref-array-di")(ref);

module.exports = ffi.Library(path.join(__dirname + "/libgpxparser.dylib"), {
  createValidGPXdoc: [voidPtr, ["string", "string"]],
  GPXtoJSON: ["string", [voidPtr]],
  routeToJSON: ["string", [voidPtr]],
  getTrackList: [voidPtr, [voidPtr]],
  getRouteList: [voidPtr, [voidPtr]],
  trackListToJSON: ["string", [voidPtr]],
  routeListToJSON: ["string", [voidPtr]],
  updateTrackName: ["int", [voidPtr, "string", "string", "string"]],
  updateRouteName: ["int", [voidPtr, "string", "string", "string"]],
  getRoutesBetween: [
    voidPtr,
    [
      voidPtr,
      ref.types.float,
      ref.types.float,
      ref.types.float,
      ref.types.float,
      ref.types.float
    ]
  ],
  getTracksBetween: [
    voidPtr,
    [
      voidPtr,
      ref.types.float,
      ref.types.float,
      ref.types.float,
      ref.types.float,
      ref.types.float
    ]
  ],
  addRouteWrapper: [
    "int",
    [
      voidPtr,
      "string",
      "string",
      ArrayType(ref.types.float),
      ArrayType(ref.types.float),
      "int"
    ]
  ],
  writeNewGPXDoc: ["bool", ["string", "string", "string", ref.types.double]],
  getRoute: [voidPtr, [voidPtr, "string"]],
  getRouteWpList: ["string", [voidPtr]]
});
