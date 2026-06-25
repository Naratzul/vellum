#!/usr/bin/env node

const { createVSIX } = require('@vscode/vsce');
const fs = require('fs');
const path = require('path');

const extensionRoot = path.join(__dirname, '..');
const monorepoRoot = path.join(extensionRoot, '..', '..', '..');
const defaultServerExe = path.join(monorepoRoot, 'bin', 'vellum-lsp', 'vellum-lsp.exe');
const stagedServerDir = path.join(extensionRoot, 'server', 'win32-x64');
const stagedServerExe = path.join(stagedServerDir, 'vellum-lsp.exe');

function parseArgs(argv) {
	let serverPath = defaultServerExe;
	for (let i = 0; i < argv.length; i++) {
		const arg = argv[i];
		if (arg === '--server-path' || arg === '-s') {
			const next = argv[++i];
			if (!next) {
				fail('Missing value for --server-path');
			}
			serverPath = path.resolve(next);
		} else if (arg === '--help' || arg === '-h') {
			console.log(`Usage: node scripts/package.js [--server-path <path>]

Stages vellum-lsp into server/win32-x64/, packages a VSIX, then removes the staged server.

Options:
  --server-path, -s   Path to vellum-lsp.exe (default: ${defaultServerExe})
`);
			process.exit(0);
		} else {
			fail(`Unknown argument: ${arg}`);
		}
	}
	return serverPath;
}

function fail(message) {
	console.error(`package.js: ${message}`);
	process.exit(1);
}

function copyServer(serverPath) {
	if (!fs.existsSync(serverPath)) {
		fail(
			`vellum-lsp executable not found at: ${serverPath}\n` +
			'Build the Release target first, e.g. cmake --build build --config Release',
		);
	}

	fs.rmSync(path.join(extensionRoot, 'server'), { recursive: true, force: true });
	fs.mkdirSync(stagedServerDir, { recursive: true });
	fs.copyFileSync(serverPath, stagedServerExe);

	const serverDir = path.dirname(serverPath);
	for (const name of fs.readdirSync(serverDir)) {
		if (!name.toLowerCase().endsWith('.dll')) {
			continue;
		}
		const src = path.join(serverDir, name);
		if (fs.statSync(src).isFile()) {
			fs.copyFileSync(src, path.join(stagedServerDir, name));
		}
	}

	console.log(`Staged server at ${stagedServerExe}`);
}

function readVersion() {
	const manifest = JSON.parse(
		fs.readFileSync(path.join(extensionRoot, 'package.json'), 'utf8'),
	);
	return manifest.version;
}

async function runVsce(version) {
	const distDir = path.join(extensionRoot, 'dist');
	fs.mkdirSync(distDir, { recursive: true });
	const outFile = path.join(distDir, `vellum-${version}.vsix`);

	await createVSIX({ cwd: extensionRoot, packagePath: outFile });

	console.log(`Created ${outFile}`);
}

function cleanStagedServer() {
	fs.rmSync(path.join(extensionRoot, 'server'), { recursive: true, force: true });
	console.log('Removed staged server/ directory');
}

const serverPath = parseArgs(process.argv.slice(2));

(async () => {
	try {
		copyServer(serverPath);
		const version = readVersion();
		await runVsce(version);
	} finally {
		cleanStagedServer();
	}
})().catch((err) => {
	console.error(err);
	process.exit(1);
});
