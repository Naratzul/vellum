/* --------------------------------------------------------------------------------------------
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */

import * as path from 'path';
import { window, ExtensionContext } from 'vscode';
import * as process from 'process'

import {
	LanguageClient,
	LanguageClientOptions,
	ServerOptions,
	TransportKind
} from 'vscode-languageclient/node';
import { assert } from 'console';

let client: LanguageClient;

function getServerExecutableName() {
	if (process.platform == "win32") {
		return "vellum-lsp.exe";
	} else if (process.platform == "darwin" || process.platform == "linux") {
		return "vellum-lsp";
	}
	assert(false, "Unsupported platform");
}

export function activate(context: ExtensionContext) {
	// The server is implemented in node
	const serverModule = context.asAbsolutePath(
		path.join('..', 'bin', 'vellum-lsp', getServerExecutableName()),
	);

	// If the extension is launched in debug mode then the debug server options are used
	// Otherwise the run options are used
	const serverOptions: ServerOptions = {
		run: { command: serverModule, transport: TransportKind.stdio },
		debug: {
			command: serverModule,
			transport: TransportKind.stdio,
		}
	};

	var output = window.createOutputChannel("Vellum Language Server")
	output.appendLine("Vellum Language Server logs:")
	output.show()

	// Options to control the language client
	const clientOptions: LanguageClientOptions = {
		// Register the server for plain text documents
		documentSelector: [{ scheme: 'file', language: 'vellum' }],
		outputChannel: output
	};

	// Create the language client and start the client.
	client = new LanguageClient(
		'languageServerExample',
		'Language Server Example',
		serverOptions,
		clientOptions
	);

	// Start the client. This will also launch the server
	client.start();
}

export function deactivate(): Thenable<void> | undefined {
	if (!client) {
		return undefined;
	}
	return client.stop();
}
