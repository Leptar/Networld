extends Node

var views: Dictionary = {} # entity_id (int) -> MeshInstance3D
var vbox: VBoxContainer
var is_server: bool = false
var camera: Camera3D
var local_entity_id: int = 0

func _ready() -> void:
	# 1. Caméra avec rotation statique
	camera = Camera3D.new()
	camera.position = Vector3(0, 7, 10)
	camera.rotation_degrees = Vector3(-35, 0, 0)
	add_child(camera)

	# 2. Connexion aux signaux réseau
	Networld.peer_connected.connect(_on_peer_connected)
	Networld.peer_disconnected.connect(_on_peer_disconnected)
	Networld.entity_spawned.connect(_on_entity_spawned)
	Networld.entity_despawned.connect(_on_entity_despawned)

	# 3. Interface Utilisateur de test
	vbox = VBoxContainer.new()
	add_child(vbox)

	var btn_dedicated := Button.new()
	btn_dedicated.text = "Démarrer en tant que DEDICATED SERVER"
	btn_dedicated.pressed.connect(_on_dedicated_pressed)
	vbox.add_child(btn_dedicated)

	var btn_listen := Button.new()
	btn_listen.text = "Démarrer en tant que LISTEN SERVER"
	btn_listen.pressed.connect(_on_listen_pressed)
	vbox.add_child(btn_listen)

	var btn_client := Button.new()
	btn_client.text = "Démarrer en tant que CLIENT"
	btn_client.pressed.connect(_on_client_pressed)
	vbox.add_child(btn_client)

func _on_dedicated_pressed() -> void:
	is_server = true
	add_child(Networld)
	if Networld.host_dedicated(7070):
		print("[DEDICATED] Démarré sur le port 7070 !")
		vbox.hide()
	else:
		push_error("[DEDICATED] Échec du démarrage.")

func _on_listen_pressed() -> void:
	is_server = true
	add_child(Networld)
	if Networld.host_listen(7070):
		print("[LISTEN] Démarré sur le port 7070 ! local_peer_id=", Networld.get_local_peer_id())
		vbox.hide()
	else:
		push_error("[LISTEN] Échec du démarrage.")

func _on_client_pressed() -> void:
	is_server = false
	add_child(Networld)
	if Networld.join("127.0.0.1", 7070):
		print("[CLIENT] Connexion à 127.0.0.1:7070...")
		vbox.hide()
	else:
		push_error("[CLIENT] Échec de connexion.")

func _on_peer_connected(peer_id: int, is_local: bool) -> void:
	print("[NET] Peer connecté : ", peer_id, " (is_local=", is_local, ")")
	if not is_server:
		return

	var spawn_pos := Vector3(randf_range(-3.0, 3.0), 0.0, randf_range(-3.0, 3.0))
	Networld.spawn_entity(1, peer_id, spawn_pos, Quaternion.IDENTITY, true)

func _on_peer_disconnected(peer_id: int) -> void:
	print("[NET] Peer déconnecté : ", peer_id)
	if is_server:
		Networld.destroy_entities_owned_by(peer_id)

func _on_entity_spawned(entity_id: int, type_id: int, owner_peer_id: int, pos: Vector3, rot: Quaternion, is_local: bool) -> void:
	if views.has(entity_id):
		return

	var cube := MeshInstance3D.new()
	cube.mesh = BoxMesh.new()
	cube.position = pos
	cube.quaternion = rot
	add_child(cube)

	var nose := MeshInstance3D.new()
	var nose_mesh := BoxMesh.new()
	nose_mesh.size = Vector3(0.3, 0.3, 0.4)
	nose.mesh = nose_mesh
	nose.position = Vector3(0, 0, 0.6)
	var nose_mat := StandardMaterial3D.new()
	nose_mat.albedo_color = Color(1.0, 0.3, 0.2)
	nose.material_override = nose_mat
	cube.add_child(nose)

	if is_local:
		local_entity_id = entity_id
		var mat := StandardMaterial3D.new()
		mat.albedo_color = Color(0.2, 0.6, 1.0)
		mat.roughness = 0.4
		mat.metallic = 0.1
		cube.material_override = mat

	views[entity_id] = cube
	Networld.bind_entity_view(entity_id, cube)
	print("[VIEW] Spawn entité ", entity_id, " (type=", type_id, ", owner=", owner_peer_id, ", is_local=", is_local, ") à ", pos)

func _on_entity_despawned(entity_id: int) -> void:
	if not views.has(entity_id):
		return

	if entity_id == local_entity_id:
		local_entity_id = 0

	var cube: MeshInstance3D = views[entity_id]
	views.erase(entity_id)
	if is_instance_valid(cube):
		cube.queue_free()

	print("[VIEW] Despawn entité ", entity_id)

func _physics_process(_delta: float) -> void:
	if local_entity_id == 0:
		return

	var dir: Vector2 = Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
	if dir.length_squared() > 0.0:
		Networld.set_local_move_input(local_entity_id, dir)

func _process(_delta: float) -> void:
	if local_entity_id != 0 and views.has(local_entity_id):
		camera.position = views[local_entity_id].position + Vector3(0, 7, 10)
