QtApplication {
	name: "Challah"

	protobuf.cpp.importPaths: ["protocol"]
	protobuf.cpp.useGrpc: true

	cpp.cppFlags: ['-Werror=return-type']
	cpp.cxxLanguageVersion: "c++17"
	cpp.debugInformation: true
	cpp.separateDebugInformation: true
	cpp.enableExceptions: true
	cpp.enableReproducibleBuilds: true
	cpp.enableRtti: true

	debugInformationInstallDir: "bin"
	installDebugInformation: true

	files: [
		"*.cpp",
		"*.hpp",
		"resources/data.qrc"
	]

	Group {
		files: ["resources/io.harmonyapp.Challah.svg"]
		qbs.install: qbs.targetOS.contains("linux")
		qbs.installDir: "share/icons/hicolor/scalable/apps"
	}

	Group {
		files: ["io.harmonyapp.Challah.appdata.xml"]
		qbs.install: qbs.targetOS.contains("linux")
		qbs.installDir: "share/metainfo"
	}

	Group {
		files: ["io.harmonyapp.Challah.desktop"]
		qbs.install: qbs.targetOS.contains("linux")
		qbs.installDir: "share/applications"
	}

	install: qbs.targetOS.contains("linux")
	installDir: "bin"

	Group {
		name: "Translation files"
		files: ["po/*.ts"]
	}
	Group {
		name: "QRC"
		fileTagsFilter: "qm"
		fileTags: "qt.core.resource_data"
		Qt.core.resourcePrefix: "/po"
	}

	Group {
		files: [
			"protocol/auth/v1/auth.proto",
			"protocol/chat/v1/channels.proto",
			"protocol/chat/v1/chat.proto",
			"protocol/chat/v1/emotes.proto",
			"protocol/chat/v1/guilds.proto",
			"protocol/chat/v1/messages.proto",
			"protocol/chat/v1/permissions.proto",
			"protocol/chat/v1/profile.proto",
			"protocol/chat/v1/streaming.proto",
			"protocol/chat/v1/postbox.proto",
			"protocol/harmonytypes/v1/types.proto",
			"protocol/mediaproxy/v1/mediaproxy.proto",
		]
		fileTags: "protobuf.grpc"
	}

	Depends { name: "bundle" }
	Depends { name: "cpp" }
	Depends { name: "protobuf.cpp" }
	Depends { name: "Qt"; submodules: ["gui", "concurrent", "widgets", "quick", "quickcontrols2", "qml"] }
}
