import * as fs from 'fs';
import * as path from 'path';
import { window, workspace, WorkspaceFolder, ExtensionContext, WorkspaceConfiguration } from 'vscode';
import * as process from 'process'

import {
	LanguageClient,
	LanguageClientOptions,
	ServerOptions,
	TransportKind
} from 'vscode-languageclient/node';

let client: LanguageClient;

function expandWorkspaceFolder(input: string, wsFolder: WorkspaceFolder | undefined): string {
	if (!wsFolder) return input;
  	return input.replace(/\$\{workspaceFolder\}/g, wsFolder.uri.fsPath);
}

function getServerPlatformDir(): string {
	if (process.platform === 'win32') {
		return 'win32-x64';
	} else if (process.platform === 'darwin') {
		return process.arch === 'arm64' ? 'darwin-arm64' : 'darwin-x64';
	} else if (process.platform === 'linux') {
		return process.arch === 'arm64' ? 'linux-arm64' : 'linux-x64';
	}
	throw new Error(`Unsupported platform: ${process.platform}`);
}

function getServerExecutableName(): string {
	if (process.platform == "win32") {
		return "vellum-lsp.exe";
	} else if (process.platform == "darwin" || process.platform == "linux") {
		return "vellum-lsp";
	}
	throw new Error(`Unsupported platform: ${process.platform}`);
}

function resolveServerModule(context: ExtensionContext, cfg: WorkspaceConfiguration): string | undefined {
	const override = cfg.get<string>('languageServerPath', '').trim();
	if (override) {
		return path.normalize(override);
	}

	let platformDir: string;
	try {
		platformDir = getServerPlatformDir();
	} catch (err) {
		const message = err instanceof Error ? err.message : String(err);
		void window.showErrorMessage(`Vellum: ${message}`);
		return undefined;
	}

	const serverDir = path.join(context.extensionPath, 'server', platformDir);
	return path.join(serverDir, getServerExecutableName());
}

export function activate(context: ExtensionContext) {
	const wsFolder =
		(window.activeTextEditor ? workspace.getWorkspaceFolder(window.activeTextEditor.document.uri) : undefined)
		?? workspace.workspaceFolders?.[0];

	const cfg = workspace.getConfiguration("vellum")
	const importPaths = cfg.get<string[]>("importPaths", [])
		.map(p => expandWorkspaceFolder(p, wsFolder))
		.map(p => path.normalize(p))

	const serverModule = resolveServerModule(context, cfg);
	if (!serverModule) {
		return;
	}

	if (!fs.existsSync(serverModule)) {
		void window.showErrorMessage(
			`Vellum language server not found at: ${serverModule}`,
		);
		return;
	}

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
