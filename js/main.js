let api = {};

function capitalize(string) {
  return string.charAt(0).toUpperCase() + string.slice(1);
}

function App() {
  this.needRender = true;

  this.effectLibrary = null;
  this.effectIndices = {};
  this.effectDescriptor = null;
  this.effectInstance = null;

  this.parameterValues = { 'foo': 42 };
  this.gui = new dat.GUI({name: 'Parameters'});
  this.gui.add(this.parameterValues, 'foo', 0, 100);

  this.dom = {
    viewer: document.getElementById('viewer'),
    pluginInput: document.getElementById('plugin-input'),
    effectIndex: document.getElementById('effect-index'),
    inputsBlock: document.getElementById('inputs'),
    parametersBlock: document.getElementById('parameters'),
    outputPointSpreadsheet: document.querySelector('#output-spreadsheets .point'),
    outputCornerSpreadsheet: document.querySelector('#output-spreadsheets .corner'),
    outputFaceSpreadsheet: document.querySelector('#output-spreadsheets .face'),
    outputMeshSpreadsheet: document.querySelector('#output-spreadsheets .mesh'),
    cookBtn: document.getElementById('cook-btn'),
    inputs: [],
    parameters: [],
  };

  this.onAnimationFrame = this.onAnimationFrame.bind(this);
  this.onCameraMoved = this.onCameraMoved.bind(this);
  this.onDomLoaded = this.onDomLoaded.bind(this);
  this.onUploadEffectLibrary = this.onUploadEffectLibrary.bind(this);
  this.onRuntimeInitialized = this.onRuntimeInitialized.bind(this);
  this.onSelectEffect = this.onSelectEffect.bind(this);
  this.onParameterChanged = this.onParameterChanged.bind(this);
  this.onInputChanged = this.onInputChanged.bind(this);
  this.cook = this.cook.bind(this);
}

App.prototype.onDomLoaded = function() {
  const scene = new THREE.Scene();
  const camera = new THREE.PerspectiveCamera( 45, window.innerWidth / window.innerHeight, 0.1, 1000 );

  const renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setSize(window.innerWidth / 2, window.innerHeight / 2);
  this.dom.viewer.appendChild(renderer.domElement);

  const controls = new THREE.OrbitControls( camera, renderer.domElement );

  camera.position.set(5, 3, 3);
  controls.update();

  const geometry = new THREE.BoxGeometry(1, 1, 1);
  const material = new THREE.MeshStandardMaterial( { color: 0x00ff00 } );
  const cube = new THREE.Mesh( geometry, material );
  scene.add( cube );

  {
    const geometry = new THREE.BufferGeometry();
    geometry.setAttribute( 'position', new THREE.Float32BufferAttribute( [], 3 ) );
    //const material = new THREE.PointsMaterial( { color: 0x0088ff, size: 0.5 } );
    //const points = new THREE.Points( geometry, material );
    const material = new THREE.MeshStandardMaterial( { color: 0x0088ff } );
    const points = new THREE.Mesh( geometry, material );
    scene.add( points );
    this.pointGeometry = geometry;
  }

  const directionalLight = new THREE.DirectionalLight( 0xffffff, 1.0 );
  directionalLight.position.set( 100, 200, 300 );
  scene.add( directionalLight );
  const directionalLight2 = new THREE.DirectionalLight( 0xffffff, 0.3 );
  directionalLight2.position.set( -300, -100, -200 );
  scene.add( directionalLight2 );

  controls.addEventListener('change', this.onCameraMoved);

  this.controls = controls;
  this.renderer = renderer;
  this.scene = scene;
  this.camera = camera;
  this.onAnimationFrame();
}

App.prototype.onAnimationFrame = function() {
  requestAnimationFrame(this.onAnimationFrame);

  this.controls.update();

  if (this.needRender) {
    this.renderer.render(this.scene, this.camera);
    this.needRender = false;
  }
}

App.prototype.onCameraMoved = function() {
  this.needRender = true;
}

App.prototype.onUploadEffectLibrary = async function(event) {
  // Load plugin file into WebAssembly's memory
  const file_data = new Uint8Array(await event.target.files[0].arrayBuffer());
  const f = Module.FS.open('plugin.wasm', 'w');
  Module.FS.write(f, file_data, 0, file_data.length);
  Module.FS.close(f);

  // 1. Loading library
  this.effectLibrary.load('plugin.wasm');
  const effectCount = this.effectLibrary.getEffectCount();
  console.log(`Found ${effectCount} effects in library`);

  const options = [];
  const o = document.createElement('option');
  o.innerText = "(Select an effect)";
  o.value = '';
  options.push(o);
  this.effectIndices = {};
  for (let i = 0 ; i < effectCount ; ++i) {
    const effectDescriptor = this.effectLibrary.getEffectDescriptor(i);
    const identifier = effectDescriptor.identifier();
    this.effectIndices[identifier] = i;
    const o = document.createElement('option');
    o.innerText = capitalize(identifier);
    o.value = identifier;
    options.push(o);
  }
  this.dom.effectIndex.replaceChildren(...options);
};

App.prototype.onSelectEffect = function(event) {
  const identifier = this.dom.effectIndex.value;
  const effectIndex = this.effectIndices[identifier];
  if (effectIndex === undefined) return;

  for (let key in this.inputMeshes) {
    const mesh = this.inputMeshes[key];
    mesh.unload();
  }

  this.effectDescriptor = this.effectLibrary.getEffectDescriptor(effectIndex);
  let status;
  status = this.effectDescriptor.load();
  console.log(`status = ${status}`);
  const inputCount = this.effectDescriptor.getInputCount();
  console.log(`inputCount = ${inputCount}`);
  const parameterCount = this.effectDescriptor.getParameterCount();
  console.log(`parameterCount = ${parameterCount}`);

  // Setup inputs UI
  const inputControllers = [];
  this.dom.parameters = [];
  this.inputMeshes = {};
  for (let i = 0 ; i < inputCount ; ++i) {
    const inputDesc = this.effectDescriptor.getInput(i);
    const identifier = inputDesc.identifier();
    if (identifier == "OfxMeshMainOutput") continue;
    const label = inputDesc.label();
    const divElement = document.createElement('div');
    const labelElement = document.createElement('label');
    labelElement.for = identifier;
    labelElement.innerText = label + " ";
    divElement.appendChild(labelElement);
    const ui = document.createElement('input');
    ui.type = "file";
    ui.name = identifier;
    ui.addEventListener('change', this.onInputChanged)
    divElement.appendChild(ui);
    inputControllers.push(divElement);
    this.dom.inputs.push(ui);
    this.inputMeshes[identifier] = new Module.Mesh();
  }
  this.dom.inputsBlock.replaceChildren(...inputControllers);

  // Setup parameter UI
  const paramControllers = [];
  this.dom.parameters = [];
  this.parameterValues = {};
  for (let i = 0 ; i < parameterCount ; ++i) {
    const identifier = this.effectDescriptor.getParameter(i).identifier();
    const divElement = document.createElement('div');
    const labelElement = document.createElement('label');
    labelElement.for = identifier;
    labelElement.innerText = identifier + " ";
    divElement.appendChild(labelElement);
    const ui = document.createElement('input');
    ui.type = "range";
    ui.name = identifier;
    ui.min = 0;
    ui.max = 5;
    ui.step = 'any';
    ui.value = i;
    ui.addEventListener('change', this.onParameterChanged)
    divElement.appendChild(ui);
    paramControllers.push(divElement);
    this.dom.parameters.push(ui);
    this.parameterValues[identifier] = i;
  }
  this.dom.parametersBlock.replaceChildren(...paramControllers);

  this.dom.cookBtn.addEventListener('click', this.cook);

  if (this.effectInstance !== null) {
    Module.destroy(this.effectInstance);
  }
  this.effectInstance = this.effectDescriptor.instantiate();
}

App.prototype.onParameterChanged = function(event) {
  console.log(`parameter changed: ${event.target.name}`)
  this.parameterValues[event.target.name] = event.target.value;
  this.cook();
}

App.prototype.onInputChanged = async function(event) {
  console.log(`input changed: ${event.target.name}`)

  // Load plugin file into WebAssembly's memory
  const filename = 'input__' + event.target.name + '.obj';
  const filedata = new Uint8Array(await event.target.files[0].arrayBuffer());
  const f = Module.FS.open(filename, 'w');
  Module.FS.write(f, filedata, 0, filedata.length);
  Module.FS.close(f);

  this.inputMeshes[event.target.name].loadObj(filename);
  const mesh = this.inputMeshes[event.target.name];
  //this.updateMesh(mesh);
  //this.updateSpreadsheet(mesh);
  this.cook();
}

App.prototype.cook = function(event) {
  console.log(`Cooking...`);
  for (let key in this.inputMeshes) {
    const mesh = this.inputMeshes[key];
    this.effectInstance.setInputMesh(key, mesh);
    const pointPositionAttrib = mesh.getAttribute("OfxMeshAttribPoint", "OfxMeshAttribPointPosition");
    console.log("input pointPositionAttrib: " + pointPositionAttrib.data().ptr);
  }
  for (let key in this.parameterValues) {
    const value = this.parameterValues[key];
    this.effectInstance.setParameter(key, value);
  }
  let status;
  status = this.effectInstance.cook();
  console.log(`status = ${status}`);

  const mesh = this.effectInstance.getOutputMesh();

  const pointPositionAttrib = mesh.getAttribute("OfxMeshAttribPoint", "OfxMeshAttribPointPosition");
  console.log("output pointPositionAttrib: " + pointPositionAttrib.data().ptr);
  
  this.updateMesh(mesh);
  this.updateSpreadsheet(mesh);
}

App.prototype.updateSpreadsheet = function(mesh) {
  let pointColumns = [{ name: "#", componentCount: 1, getData: (i) => i }];
  let cornerColumns = [{ name: "#", componentCount: 1, getData: (i) => i }];
  let faceColumns = [{ name: "#", componentCount: 1, getData: (i) => i }];
  let meshColumns = [{ name: "#", componentCount: 1, getData: (i) => i }];
  const attributeCount = mesh.attributeCount();
  for (let i = 0 ; i < attributeCount ; ++i) {
    const attrib = mesh.getAttributeByIndex(i);
    const identifier = attrib.identifier();
    const attachment = attrib.attachment();
    const componentCount = attrib.componentCount();
    const byteStride = attrib.byteStride();
    const type = attrib.type();

    let columns, elementCount;
    if (attachment == "OfxMeshAttribPoint") {
      columns = pointColumns;
      elementCount = mesh.pointCount();
    } else if (attachment == "OfxMeshAttribCorner") {
      columns = cornerColumns;
      elementCount = mesh.cornerCount();
    } else if (attachment == "OfxMeshAttribFace") {
      columns = faceColumns;
      elementCount = mesh.faceCount();
    } else {
      columns = meshColumns;
      elementCount = 1;
    }

    const dataView = new DataView(Module.HEAP8.buffer, attrib.data().ptr);

    let getValue, componentByteSize;
    if (type == "OfxMeshAttribTypeUByte") {
      getValue = dataView.getUint8.bind(dataView);
      componentByteSize = 1;
    } else if (type == "OfxMeshAttribTypeInt") {
      getValue = dataView.getInt32.bind(dataView);
      componentByteSize = 4;
    } else if (type == "OfxMeshAttribTypeFloat") {
      getValue = dataView.getFloat32.bind(dataView);
      componentByteSize = 4;
    } else {
      console.error("Unknown attribute type: " + type);
    }

    const getData = (index, component) => getValue(byteStride * index + componentByteSize * component, true);
    
    if (mesh.constantFaceSize() > -1 && attachment == "OfxMeshAttribFace" && identifier == "OfxMeshAttribFaceSize") {
      data = new Int32Array(mesh.faceCount());
      data.fill(mesh.constantFaceSize());
    }

    let displayedIdentifier = identifier;
    if (identifier.startsWith(attachment)) {
      displayedIdentifier = identifier.substring(attachment.length);
    }

    columns.push({
      name: displayedIdentifier,
      componentCount: componentCount,
      getData: getData,
    });
  }
  
  updateSpreadsheet(this.dom.outputPointSpreadsheet, pointColumns, mesh.pointCount());
  updateSpreadsheet(this.dom.outputCornerSpreadsheet, cornerColumns, mesh.cornerCount());
  updateSpreadsheet(this.dom.outputFaceSpreadsheet, faceColumns, mesh.faceCount());
  updateSpreadsheet(this.dom.outputMeshSpreadsheet, meshColumns, 1);
}

App.prototype.updateMesh = function(mesh) {
  const pointCount = mesh.pointCount();
  const cornerCount = mesh.cornerCount();
  const faceCount = mesh.faceCount();
  const constantFaceSize = mesh.constantFaceSize();

  console.log(mesh.isValid());
  console.log(` - ${pointCount} points`);
  console.log(` - ${cornerCount} corners`);
  console.log(` - ${faceCount} faces`);
  console.log(` - constantFaceSize = ${constantFaceSize}`);

  const pointPositionAttrib = mesh.getAttribute("OfxMeshAttribPoint", "OfxMeshAttribPointPosition");
  const pointPositionStride = pointPositionAttrib.byteStride();
  const pointPositionDataView = new DataView(Module.HEAP8.buffer, pointPositionAttrib.data().ptr);
  const getPointPosition = (index, component) => pointPositionDataView.getFloat32(pointPositionStride * index + 4 * component, true);

  const cornerPointAttrib = mesh.getAttribute("OfxMeshAttribCorner", "OfxMeshAttribCornerPoint");
  const cornerPointStride = cornerPointAttrib.byteStride();
  const cornerPointDataView = new DataView(Module.HEAP8.buffer, cornerPointAttrib.data().ptr);
  const getCornerPoint = index => cornerPointDataView.getInt32(cornerPointStride * index, true);

  let getFaceSize;
  if (mesh.constantFaceSize() == -1) {
    const faceSizeAttrib = mesh.getAttribute("OfxMeshAttribFace", "OfxMeshAttribFaceSize");
    const faceSizeStride = faceSizeAttrib.byteStride();
    const faceSizeDataView = new DataView(Module.HEAP8.buffer, faceSizeAttrib.data().ptr);
    getFaceSize = index => faceSizeDataView.getInt32(faceSizeStride * index, true);
  } else {
    getFaceSize = index => mesh.constantFaceSize();
  }

  // Convert to ThreeJs-ready triangles
  let triangleCount = 0;
  for (let f = 0 ; f < faceCount ; ++f) {
    const size = getFaceSize(f);
    if (size == 3) {
      triangleCount += 1;
    } else if (size == 4) {
      triangleCount += 2;
    } else {
      console.error(`Unsupported face size in face #${f}: ${size}`);
    }
  }
  const tesselatedCornerPointData = new Uint32Array(3 * triangleCount);
  let triangleIndex = 0;
  let cornerIndex = 0;
  for (let f = 0 ; f < faceCount ; ++f) {
    const size = getFaceSize(f);

    for (let k = 0 ; k < 3 ; ++k) {
      tesselatedCornerPointData[3 * triangleIndex + k] = getCornerPoint(cornerIndex + k);
    }
    ++triangleIndex;

    if (size == 4) {
      for (let k = 0 ; k < 3 ; ++k) {
        tesselatedCornerPointData[3 * triangleIndex + k] = getCornerPoint(cornerIndex + (k + 2) % 4);
      }
      ++triangleIndex;
    }

    cornerIndex += size;
  }

  // Expand the index buffer and compute normals
  const tesselatedPointPositionData = new Float32Array(3 * 3 * triangleCount);
  const tesselatedPointNormalData = new Float32Array(3 * 3 * triangleCount);
  const pos = [new THREE.Vector3(), new THREE.Vector3(), new THREE.Vector3()];
  const ab = new THREE.Vector3();
  const ac = new THREE.Vector3();
  const normal = new THREE.Vector3();
  for (let t = 0 ; t < triangleCount ; ++t) {
    for (let c = 0 ; c < 3 ; ++c) {
      const p = tesselatedCornerPointData[3 * t + c];
      for (let k = 0 ; k < 3 ; ++k) {
        const value = getPointPosition(p, k);
        pos[c].setComponent(k, value);
        tesselatedPointPositionData[3 * (3 * t + c) + k] = value;
      }
    }
    ab.subVectors(pos[1], pos[0]);
    ac.subVectors(pos[2], pos[0]);
    normal.crossVectors(ab, ac);
    normal.normalize();
    for (let c = 0 ; c < 3 ; ++c) {
      for (let k = 0 ; k < 3 ; ++k) {
        tesselatedPointNormalData[3 * (3 * t + c) + k] = normal.getComponent(k);
      }
    }
  }

  // Update geometry
  this.pointGeometry.setAttribute('position', new THREE.BufferAttribute(tesselatedPointPositionData, 3));
  this.pointGeometry.setAttribute('normal', new THREE.BufferAttribute(tesselatedPointNormalData, 3));
  this.needRender = true;
}

App.prototype.onRuntimeInitialized = async function(event) {
  console.log("WebAssembly module is ready.");
  api = {
    test: Module._test,
  }

  app.effectLibrary = new Module.EffectLibrary();

  this.dom.pluginInput.addEventListener('change', app.onUploadEffectLibrary);
  this.dom.effectIndex.addEventListener('change', app.onSelectEffect);
}

const app = new App();
Module.onRuntimeInitialized = app.onRuntimeInitialized;
document.addEventListener('DOMContentLoaded', app.onDomLoaded);
