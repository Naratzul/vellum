import * as path from 'path';
import { window, workspace, WorkspaceFolder, ExtensionContext } from 'vscode';
import * as process from 'process'

import {
	LanguageClient,
	LanguageClientOptions,
	ServerOptions,
	TransportKind
} from 'vscode-languageclient/node';
import { assert } from 'console';

let client: LanguageClient;

function expandWorkspaceFolder(input: string, wsFolder: WorkspaceFolder | undefined): string {
	if (!wsFolder) return input;
  	return input.replace(/\$\{workspaceFolder\}/g, wsFolder.uri.fsPath);
}

function getServerExecutableName() {
	if (process.platform == "win32") {
		return "vellum-lsp.exe";
	} else if (process.platform == "darwin" || process.platform == "linux") {
		return "vellum-lsp";
	}
	assert(false, "Unsupported platform");
}

export function activate(context: ExtensionContext) {
	const wsFolder =
		(window.activeTextEditor ? workspace.getWorkspaceFolder(window.activeTextEditor.document.uri) : undefined)
		?? workspace.workspaceFolders?.[0];

	const cfg = workspace.getConfiguration("vellum")
	const importPaths = cfg.get<string[]>("importPaths", [])
		.map(p => expandWorkspaceFolder(p, wsFolder))
		.map(p => path.normalize(p))

	const serverModule = context.asAbsolutePath(
		path.join('..', '..', '..', 'bin', 'vellum-lsp', getServerExecutableName()),
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
		documentSelector: [{ scheme: 'file', language: 'vellum' }],
		initializationOptions: {
			importPaths: importPaths
		},
		synchronize: {
			configurationSection: 'vellum',
		},
		workspaceFolder: wsFolder,
		outputChannel: output,
		middleware: {
			provideDefinition: (document, position, token, next) => {
				output.appendLine(
					`[client] textDocument/definition ${document.uri.fsPath} ` +
					`@ ${position.line}:${position.character}`,
				);
				return next(document, position, token);
			},
			provideCompletionItem: (document, position, context, token, next) => {
				output.appendLine(
					`[client] textDocument/completion ${document.uri.fsPath} ` +
					`@ ${position.line}:${position.character}`,
				);
				return next(document, position, context, token);
			},
		},
	};

	// Create the language client and start the client.
	client = new LanguageClient(
		'vellum',
		'vellum',
		serverOptions,
		clientOptions
	);

	// Start the client. This will also launch the server
	void client.start().then(() => {
		const caps = client.initializeResult?.capabilities;
		output.appendLine(
			`[client] server definitionProvider: ${JSON.stringify(caps?.definitionProvider ?? null)}`,
		);
		output.appendLine(
			`[client] server completionProvider: ${JSON.stringify(caps?.completionProvider ?? null)}`,
		);
	});
}

export function deactivate(): Thenable<void> | undefined {
	if (!client) {
		return undefined;
	}
	return client.stop();
}
