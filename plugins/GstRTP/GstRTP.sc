GstRTP {
	classvar map;

	*initClass {
		map = ();
	}

	*addOut { arg key, host, port, server;
		var max_tries = 10;
		var id;
		if (server.isNil) {
			server = Server.default;
		};
		id = 99999.rand;

		while { map.values.includes(id) and: { max_tries > 0 } } { id = 99999.rand; max_tries = max_tries - 1; };
		map[key] = id;
		server.sendMsg('/cmd', '/gstrtp_set_out', id, host, port);
	}

	*at { arg key;
		^map.at(key);
	}
}

GstRTPOut : UGen {
	*ar { |key, input|
		var id = GstRTP.at(key);
		var channels = 1;
		if (id.isNil) {
			^Error("No key % registered with GstRTPOut".format(key)).throw;
		};

		if (input.isArray.not) {
			input = [input];
		};

		channels = input.size;
		if ([1,2].includes(channels).not) {
			^Error("Input should be mono or stereo").throw;
		};

		^this.multiNew('audio', channels, id, *input);
	}
	checkInputs {
		/* TODO */
		^this.checkValidInputs;
	}
}
